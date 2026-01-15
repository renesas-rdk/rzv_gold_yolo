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
#pragma once

#include <memory>
#include <opencv2/opencv.hpp>
#include <vector>

#include "rzv_model/base_model.hpp"

namespace rzv_model
{

struct GOLDYOLODetection
{
  cv::Rect bbox;
  int class_id;
  float confidence;
  bool is_valid = false;
  std::string class_name;
};

struct GOLDYOLODetectionResult : public ModelResult
{
  std::vector<GOLDYOLODetection> detections;
};

class GoldYoloModel : public BaseModel
{
public:
  GoldYoloModel();
  virtual ~GoldYoloModel();

  void set_class_names(const std::vector<std::string> & class_names);
  void set_confidence_threshold(float threshold);
  void set_iou_threshold(float threshold);

  float get_confidence_threshold() const;
  float get_iou_threshold() const;
  const std::vector<std::string> & get_class_names() const;

  // Usage examples for running the model:
  // 1. Using the templated run method (recommended):
  //    auto result = model->run<rzv_model::GOLDYOLODetectionResult>(input);
  //
  // 2. Alternatively, if you're working with the base class:
  //    auto base_result = model->run(input);
  //    auto typed_result = dynamic_cast<rzv_model::GOLDYOLODetectionResult*>(base_result.get());
protected:
  cv::Mat preprocess(const ModelInput & input) override;
  cv::Mat fallback_preprocess(const ModelInput & input) override;
  std::unique_ptr<ModelResult> postprocess(const std::vector<cv::Mat> & output_tensors) override;

private:
  class Impl;
  std::unique_ptr<Impl> pimpl_;
};

}  // namespace rzv_model
