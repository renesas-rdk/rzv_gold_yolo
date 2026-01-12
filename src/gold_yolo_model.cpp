// ********************************************************************************************************************
// Copyright [2025] Renesas Electronics Corporation and/or its licensors. All Rights Reserved.
//
// The contents of this file (the "contents") are proprietary and confidential to Renesas Electronics Corporation
// and/or its licensors ("Renesas") and subject to statutory and contractual protections.
//
// Unless otherwise expressly agreed in writing between Renesas and you: 1) you may not use, copy, modify, distribute,
// display, or perform the contents; 2) you may not use any name or mark of Renesas for advertising or publicity
// purposes or in connection with your use of the contents; 3) RENESAS MAKES NO WARRANTY OR REPRESENTATIONS ABOUT THE
// SUITABILITY OF THE CONTENTS FOR ANY PURPOSE; THE CONTENTS ARE PROVIDED "AS IS" WITHOUT ANY EXPRESS OR IMPLIED
// WARRANTY, INCLUDING THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, AND
// NON-INFRINGEMENT; AND 4) RENESAS SHALL NOT BE LIABLE FOR ANY DIRECT, INDIRECT, SPECIAL, OR CONSEQUENTIAL DAMAGES,
// INCLUDING DAMAGES RESULTING FROM LOSS OF USE, DATA, OR PROJECTS, WHETHER IN AN ACTION OF CONTRACT OR TORT, ARISING
// OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THE CONTENTS. Third-party contents included in this file may
// be subject to different terms.
// ********************************************************************************************************************
#include <algorithm>
#include <cmath>

#include "rzv_gold_yolo/gold_yolo_model.hpp"
#include "rzv_model/utils.hpp"

namespace rzv_model
{

class GoldYoloModel::Impl
{
public:
  int num_classes = 20;  // Default to Pascal VOC classes
  float conf_threshold = 0.25f;
  float iou_threshold = 0.45f;
  std::vector<std::string> class_names;
};

GoldYoloModel::GoldYoloModel() : BaseModel(), pimpl_(std::make_unique<Impl>())
{
  RCLCPP_INFO(get_logger(), "Gold-Yolo model instance created with default settings");

  // Default class names (can be overridden with set_class_names)
  pimpl_->class_names = {"empty"};
}

GoldYoloModel::~GoldYoloModel() = default;

void GoldYoloModel::set_class_names(const std::vector<std::string> & class_names)
{
  pimpl_->class_names = class_names;
  pimpl_->num_classes = static_cast<int>(class_names.size());
  RCLCPP_INFO(get_logger(), "Set %d class names for Gold-Yolo model", pimpl_->num_classes);
}

void GoldYoloModel::set_confidence_threshold(float threshold)
{
  if (threshold >= 0.0f && threshold <= 1.0f) {
    pimpl_->conf_threshold = threshold;
    RCLCPP_INFO(get_logger(), "Set confidence threshold to %.2f", threshold);
  } else {
    RCLCPP_WARN(
      get_logger(), "Invalid confidence threshold: %.2f (must be between 0.0 and 1.0)", threshold);
  }
}

void GoldYoloModel::set_iou_threshold(float threshold)
{
  if (threshold >= 0.0f && threshold <= 1.0f) {
    pimpl_->iou_threshold = threshold;
    RCLCPP_INFO(get_logger(), "Set IoU threshold to %.2f", threshold);
  } else {
    RCLCPP_WARN(
      get_logger(), "Invalid IoU threshold: %.2f (must be between 0.0 and 1.0)", threshold);
  }
}

float GoldYoloModel::get_confidence_threshold() const { return pimpl_->conf_threshold; }

float GoldYoloModel::get_iou_threshold() const { return pimpl_->iou_threshold; }

const std::vector<std::string> & GoldYoloModel::get_class_names() const
{
  return pimpl_->class_names;
}

cv::Mat GoldYoloModel::preprocess(const ModelInput & input)
{
  return BaseModel::preprocess(input);
}
cv::Mat GoldYoloModel::fallback_preprocess(const ModelInput & input)
{
  return BaseModel::software_preprocess(input, false);
}

std::unique_ptr<ModelResult> GoldYoloModel::postprocess(
  const std::vector<cv::Mat> & output_tensors)
{
  // This method handles the Unified Detection Tensor format:
  // - Shape: [batch, num_boxes, box_data] where box_data contains:
  //   Positions 0-3: Bounding box parameters (cx, cy, width, height) in absolute pixel coordinates
  //   Position 4: Object confidence score
  //   Positions 5+: Class probabilities for each object class

  auto result = std::make_unique<GOLDYOLODetectionResult>();

  if (output_tensors.empty()) {
    RCLCPP_ERROR(get_logger(), "Empty output from Gold-Yolo model");
    return result;
  }

  const cv::Mat & tensor = output_tensors[0];  // We expect a single tensor in the unified format

  if (tensor.empty()) {
    RCLCPP_ERROR(get_logger(), "Empty tensor received from model");
    return result;
  }

  RCLCPP_DEBUG(
    get_logger(), "Processing unified detection tensor with shape [%d, %d, %d]", tensor.size[0],
    tensor.size[1], tensor.size[2]);

  // Prepare vectors for detection data
  std::vector<cv::Rect2f> bboxes;
  std::vector<float> confidences;
  std::vector<int> class_ids;
  std::vector<std::string> class_names;

  // The tensor should have shape [batch, num_boxes, box_data]
  // Where box_data contains x, y, w, h, conf, and class probabilities
  if (tensor.dims == 3) {
    int batch_size = tensor.size[0];
    int num_boxes = tensor.size[1];
    int box_data_size = tensor.size[2];

    // Class count is box_data_size - 5 (x, y, w, h, conf)
    int num_classes = box_data_size - 5;

    RCLCPP_DEBUG(
      get_logger(),
      "Unified tensor format: batch_size=%d, num_boxes=%d, box_data_size=%d, num_classes=%d",
      batch_size, num_boxes, box_data_size, num_classes);

    // Get pointer to tensor data
    const float * data_ptr = tensor.ptr<float>();

    // Process each detection box
    for (int i = 0; i < num_boxes; ++i) {
      // Get the base index for this box
      int box_idx = i * box_data_size;

      // Extract object confidence
      float obj_conf = data_ptr[box_idx + 4];

      // Skip if confidence is below threshold
      if (obj_conf < get_confidence_threshold()) {
        continue;
      }

      // Find class with highest confidence
      float max_class_conf = 0.0f;
      int max_class_id = -1;

      for (int c = 0; c < num_classes; ++c) {
        float class_conf = data_ptr[box_idx + 5 + c];
        if (class_conf > max_class_conf) {
          max_class_conf = class_conf;
          max_class_id = c;
        }
      }

      // Compute final confidence
      float confidence = obj_conf * max_class_conf;

      // Skip if confidence is too low
      if (max_class_id == -1 || confidence < get_confidence_threshold()) {
        continue;
      }

      // Extract the bounding box coordinates (in absolute pixel coordinates)
      float cx = data_ptr[box_idx + 0];      // center x
      float cy = data_ptr[box_idx + 1];      // center y
      float width = data_ptr[box_idx + 2];   // width
      float height = data_ptr[box_idx + 3];  // height

      // Convert center coordinates to top-left coordinates
      float x = cx - width / 2;
      float y = cy - height / 2;
      float w = width;
      float h = height;

      // Filter out small boxes
      if (w < 5.0f || h < 5.0f) {
        continue;
      }

      bboxes.push_back(cv::Rect2f(x, y, w, h));
      confidences.push_back(confidence);
      class_ids.push_back(max_class_id);

      // Get class name
      std::string class_name;
      if (max_class_id >= 0 && max_class_id < static_cast<int>(get_class_names().size())) {
        class_name = get_class_names()[max_class_id];
      } else {
        class_name = "class_" + std::to_string(max_class_id);
      }
      class_names.push_back(class_name);
    }
  } else {
    RCLCPP_WARN(
      get_logger(), "Unexpected tensor format. Expected dims=3, got dims=%d", tensor.dims);
    return result;
  }

  // Apply Non-Maximum Suppression
  std::vector<int> indices = Utils::non_maximum_suppression(
    bboxes, confidences, get_confidence_threshold(), get_iou_threshold());
  RCLCPP_DEBUG(get_logger(), "NMS returned %zu indices", indices.size());

  // Process the kept detections
  float best_score = 0.0f;
  for (int idx : indices) {
    // Convert detection to original image space
    float x1 = bboxes[idx].x;
    float y1 = bboxes[idx].y;
    float x2 = x1 + bboxes[idx].width;
    float y2 = y1 + bboxes[idx].height;

    // Map coordinates back to original image space
    cv::Point2f top_left = map_coordinates_to_original(cv::Point2f(x1, y1));
    cv::Point2f bottom_right = map_coordinates_to_original(cv::Point2f(x2, y2));

    // Calculate box width and height
    float box_w = bottom_right.x - top_left.x;
    float box_h = bottom_right.y - top_left.y;

    // Create final detection
    GOLDYOLODetection detection;
    detection.bbox = cv::Rect(
      static_cast<int>(std::round(top_left.x)), static_cast<int>(std::round(top_left.y)),
      static_cast<int>(std::round(box_w)), static_cast<int>(std::round(box_h)));
    detection.class_id = class_ids[idx];
    detection.confidence = confidences[idx];
    detection.class_name = class_names[idx];
    detection.is_valid = true;

    result->detections.push_back(detection);
    best_score = std::max(best_score, detection.confidence);

    RCLCPP_DEBUG(
      get_logger(), "Final detection: %s at: %d, %d, %d, %d with score %0.2f",
      detection.class_name.c_str(), detection.bbox.x, detection.bbox.y, detection.bbox.width,
      detection.bbox.height, detection.confidence);
  }

  // Set highest confidence as overall score
  result->score = indices.empty() ? 0.0f : best_score;

  if (indices.empty()) {
    RCLCPP_DEBUG(get_logger(), "No detections passed NMS");
  }

  return result;
}

}  // namespace rzv_model
