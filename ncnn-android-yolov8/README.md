# ncnn-android-yolov8

The yolov8 object detection

This is a sample ncnn android project, it depends on ncnn library and opencv

首先 pt模型导出onnx
yolo export model=best_n.pt format=onnx simplify=True opset=12 half=True

注意 上面后面所加的三个参数十分重要；不然识别不了并且还根本找不到问题 simplify=True opset=12 half=True 就这三个参数

cd /opt/homebrew/Cellar/ncnn/20240102_2/bin/

onnx2ncnn /Users/lipengjun/yolo/yolov8n.onnx /Users/lipengjun/yolo/yolov8n.param /Users/lipengjun/yolo/yolov8n.bin
onnx2ncnn /Users/lipengjun/yolo/nanjing0303.onnx /Users/lipengjun/yolo/nanjing0303.param /Users/lipengjun/yolo/nanjing0303.bin
onnx2ncnn best_n.onnx best_n.param best_n.bin

弃用：cd GitHub/onnx2caffe
弃用：python3 convertCaffe.py /Users/lipengjun/yolo/nanjing0303.onnx /Users/lipengjun/yolo/nanjing0303.prototxt /Users/lipengjun/yolo/nanjing0303.caffemodel

https://github.com/Tencent/ncnn

https://github.com/nihui/opencv-mobile


## how to build and run
### step1
https://github.com/Tencent/ncnn/releases

* Download ncnn-YYYYMMDD-android-vulkan.zip or build ncnn for android yourself
* Extract ncnn-YYYYMMDD-android-vulkan.zip into **app/src/main/jni** and change the **ncnn_DIR** path to yours in **app/src/main/jni/CMakeLists.txt**

### step2
https://github.com/nihui/opencv-mobile

* Download opencv-mobile-XYZ-android.zip
* Extract opencv-mobile-XYZ-android.zip into **app/src/main/jni** and change the **OpenCV_DIR** path to yours in **app/src/main/jni/CMakeLists.txt**

### step3
* Open this project with Android Studio, build it and enjoy!

## some notes
* Android ndk camera is used for best efficiency
* Crash may happen on very old devices for lacking HAL3 camera interface
* All models are manually modified to accept dynamic input shape
* Most small models run slower on GPU than on CPU, this is common
* FPS may be lower in dark environment because of longer camera exposure time

## screenshot
![](screenshot.png)

## Reference：  
https://github.com/nihui/ncnn-android-nanodet  
https://github.com/Tencent/ncnn  
https://github.com/ultralytics/assets/releases/tag/v0.0.0
