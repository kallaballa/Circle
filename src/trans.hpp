/*
 * trans.hpp
 *
 *  Created on: Jul 29, 2017
 *      Author: elchaschab
 */

#ifndef SRC_TRANS_HPP_
#define SRC_TRANS_HPP_

#include <vector>
#include <iostream>
#include <opencv2/opencv.hpp>

class TransMatrices;


class ChangeMatrixCommand : public NLCommand {
protected:
  std::vector<cv::Mat*> docMat_;
  std::vector<cv::Mat*> before_;
  std::vector<cv::Mat*> after_;

public:
  ChangeMatrixCommand(std::vector<cv::Mat*>& docMat, const std::vector<cv::Mat*>& changeMat ) : docMat_(docMat), before_(5), after_(5) {
    std::cerr << "###add###" << std::endl;
    std::cerr << "-------" << std::endl;
    for(size_t i = 0; i < docMat_.size(); ++i) {
      before_[i] = new cv::Mat(2,3,CV_64F, cvScalar(0.0));
      docMat_[i]->copyTo(*before_[i]);
    }
    for(size_t i = 0; i < docMat_.size(); ++i) {
      std::cerr << (*before_[i]) << std::endl;
    }


    for(size_t i = 0; i < changeMat.size(); ++i) {
      after_[i] = new cv::Mat(2,3,CV_64F, cvScalar(0.0));
      changeMat[i]->copyTo(*after_[i]);
    }
    for(size_t i = 0; i < docMat_.size(); ++i) {
      std::cerr << (*after_[i]) << std::endl;
    }
    std::cerr << "###add###" << std::endl;
  }
  ~ChangeMatrixCommand() {
    std::cerr << "destroy" << std::endl;
    for(size_t i = 0; i < before_.size(); ++i) {
      delete before_[i];
    }

    for(size_t i = 0; i < after_.size(); ++i) {
      delete after_[i];
    }
  }

  void execute() {
    std::cerr << "###do###" << std::endl;
    for(size_t i = 0; i < docMat_.size(); ++i) {
      std::cerr << (*docMat_[i]) << std::endl;
    }
    std::cerr << "-------" << std::endl;
    for(size_t i = 0; i < docMat_.size(); ++i) {
      after_[i]->copyTo(*docMat_[i]);
    }
    for(size_t i = 0; i < docMat_.size(); ++i) {
      std::cerr << (*docMat_[i]) << std::endl;
    }
    std::cerr << "###do###" << std::endl;
  }

  void undo() {
    std::cerr << "##undo##" << std::endl;
    for(size_t i = 0; i < docMat_.size(); ++i) {
      std::cerr << (*docMat_[i]) << std::endl;
    }
    std::cerr << "-------" << std::endl;
    for(size_t i = 0; i < docMat_.size(); ++i) {
      before_[i]->copyTo(*docMat_[i]);
    }
    for(size_t i = 0; i < docMat_.size(); ++i) {
      std::cerr << (*docMat_[i]) << std::endl;
    }
    std::cerr << "##undo##" << std::endl;
  }
};

class TransMatrices  {
public:
  enum TransType {
    Rotation,
    ShearX,
    ShearY,
    StretchX,
    StretchY,
    Zoom
  };

  NLHistory history_;
  cv::Mat rotMat_;
  cv::Mat shearXMat_;
  cv::Mat shearYMat_;
  cv::Mat stretchXMat_;
  cv::Mat stretchYMat_;
  cv::Mat zoomMat_;
  cv::Mat tmpRotMat_;
  cv::Mat tmpShearXMat_;
  cv::Mat tmpShearYMat_;
  cv::Mat tmpStretchXMat_;
  cv::Mat tmpStretchYMat_;
  cv::Mat tmpZoomMat_;

  std::vector<cv::Mat*> matVector_ = {&rotMat_, &shearXMat_, &shearYMat_, &stretchXMat_, &stretchYMat_, &zoomMat_};
  std::vector<cv::Mat*> tmpMatVector_ = {&tmpRotMat_, &tmpShearXMat_, &tmpShearYMat_, &tmpStretchXMat_, &tmpStretchYMat_, &tmpZoomMat_};

  size_t transTypeToIndex(TransType t) {
    switch(t) {
    case Rotation:
      return 0;
      break;
    case ShearX:
      return 1;
      break;
    case ShearY:
      return 2;
      break;
    case StretchX:
      return 3;
      break;
    case StretchY:
      return 4;
      break;
    case Zoom:
      return 5;
      break;
    }
    assert(false);
    return 0;
  }
public:
  std::vector<TransType> transTypes_ = {Rotation, ShearX, ShearY, StretchX, StretchY, Zoom };

  TransMatrices() :
    rotMat_(2,3,CV_64F, cvScalar(0.0)),
    shearXMat_(2,3,CV_64F, cvScalar(0.0)),
    shearYMat_(2,3,CV_64F, cvScalar(0.0)),
    stretchXMat_(2,3,CV_64F, cvScalar(0.0)),
    stretchYMat_(2,3,CV_64F, cvScalar(0.0)),
    zoomMat_(2,3,CV_64F, cvScalar(0.0)),
    tmpRotMat_(2,3,CV_64F, cvScalar(0.0)),
    tmpShearXMat_(2,3,CV_64F, cvScalar(0.0)),
    tmpShearYMat_(2,3,CV_64F, cvScalar(0.0)),
    tmpStretchXMat_(2,3,CV_64F, cvScalar(0.0)),
    tmpStretchYMat_(2,3,CV_64F, cvScalar(0.0)),
    tmpZoomMat_(2,3,CV_64F, cvScalar(0.0)) {

    this->reset();
  }

  virtual ~TransMatrices(){
  }

  void set(const TransType& type, const int& x, const int& y, const double& val) {
    tmpMatVector_[transTypeToIndex(type)]->at<double>(x,y) = val;
  }

  const double get(const TransType& type, const int& x, const int& y) {
    return tmpMatVector_[transTypeToIndex(type)]->at<double>(x,y);
  }

  cv::Mat& getMat(TransType type) {
    return *(tmpMatVector_[transTypeToIndex(type)]);
  }

  void saveState() {
    std::cerr << "#saveState#" << std::endl;
    for(size_t i = 0; i < matVector_.size(); ++i) {
      std::cerr << *tmpMatVector_[i] << std::endl;
    }
    std::cerr << "-----------" << std::endl;
    history_.add(new ChangeMatrixCommand(matVector_, tmpMatVector_), true);
    for(size_t i = 0; i < matVector_.size(); ++i) {
      std::cerr << *tmpMatVector_[i] << std::endl;
    }
    std::cerr << "#saveState#" << std::endl;
  }

  void undo() {
    history_.undo();
    for(size_t i = 0; i < matVector_.size(); ++i) {
      matVector_[i]->copyTo(*tmpMatVector_[i]);
    }
  }

  void redo() {
    history_.redo();
    for(size_t i = 0; i < matVector_.size(); ++i) {
      (*tmpMatVector_[i]) = (*matVector_[i]);
    }
  }

  void reset() {
    for(auto mat: tmpMatVector_) {
      mat->at<double>(0,0) = 1;
      mat->at<double>(1,1) = 1;
      mat->at<double>(0,1) = 0;
      mat->at<double>(1,0) = 0;
    }

    for(auto matPtr: matVector_) {
      matPtr->at<double>(0,0) = 1;
      matPtr->at<double>(1,1) = 1;
      matPtr->at<double>(0,1) = 0;
      matPtr->at<double>(1,0) = 0;
    }
    history_.clear();
  }
};


#endif /* SRC_TRANS_HPP_ */
