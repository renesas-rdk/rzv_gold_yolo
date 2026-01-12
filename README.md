# RZV Gold YOLO

A C++ package providing Gold-YOLO object detection models optimized for Renesas RZ/V processors with DRP-AI acceleration support.

## Overview

This package implements Gold-YOLO Detection for standard object detection with axis-aligned bounding boxes

The classes are designed to be used via the ``rzv_model::BaseModel`` interface (from dependency rzv_model) and override the YOLOX-specific post-processing.

## Features

* Gold-YOLO Detect model support (axis-aligned bounding boxes)
* Implements ``rzv_model::GoldYoloModel`` deriving from ``rzv_model::BaseModel``

## Usage

For preparing the AI model files and converting to the DRP-AI compatible format, please refer to the ``TBD`` documentation.

### Basic YOLOX Detection

```cpp
#include "rzv_gold_yolo/gold_yolo_model.hpp"

// Create model instance
auto model = std::make_unique<rzv_model::GoldYoloModel>();

// Configure model
std::vector<std::string> class_names = {"person", "car", "bicycle", "dog","cat"}; // Must match model training
model->set_class_names(class_names);
model->set_confidence_threshold(0.5f);
model->set_iou_threshold(0.4f);

// Load model
model->load("path/to/gold_yolo_model");

// Run inference
cv::Mat input_image = cv::imread("image.jpg");
auto object_image_input = rzv_model::ModelInput{input_image, cv::Rect(0, 0, input_image.cols, input_image.rows)};
auto result = model->run<rzv_model::GOLDYOLODetectionResult>(object_image_input);

// Process results
for (const auto & detection : result->detections) {
  if (detection.is_valid) {
    std::cout << "Class: " << detection.class_name << " Confidence: " << detection.confidence
              << " BBox: [" << detection.bbox.x << ", " << detection.bbox.y << ", "
              << detection.bbox.width << ", " << detection.bbox.height << "]" << std::endl;
  }
}
```

## Configuration Options

### Model Parameters

```cpp
// Set confidence threshold (0.0 - 1.0)
model->set_confidence_threshold(0.5f);

// Set NMS threshold (0.0 - 1.0)
model->set_iou_threshold(0.4f);

// Set custom class names, the size must match the model's trained classes
std::vector<std::string> custom_classes = {"class1", "class2", "class3"};
model->set_class_names(custom_classes);
```

## Dependencies
- rzv_model (base model interface)
- OpenCV

## License
This package is licensed under Apache 2.0.