// Tencent is pleased to support the open source community by making ncnn available.
//
// Copyright (C) 2021 THL A29 Limited, a Tencent company. All rights reserved.
//
// Licensed under the BSD 3-Clause License (the "License"); you may not use this file except
// in compliance with the License. You may obtain a copy of the License at
//
// https://opensource.org/licenses/BSD-3-Clause
//
// Unless required by applicable law or agreed to in writing, software distributed
// under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
// CONDITIONS OF ANY KIND, either express or implied. See the License for the
// specific language governing permissions and limitations under the License.

#include <android/asset_manager_jni.h>
#include <android/native_window_jni.h>
#include <android/native_window.h>

#include <android/log.h>

#include <jni.h>

#include <string>
#include <vector>

#include <platform.h>
#include <benchmark.h>

#include "yolo.h"

#include "ndkcamera.h"

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#if __ARM_NEON
#include <arm_neon.h>
#endif // __ARM_NEON

static int draw_unsupported(cv::Mat &rgb) {
    const char text[] = "unsupported";

    int baseLine = 0;
    cv::Size label_size = cv::getTextSize(text, cv::FONT_HERSHEY_SIMPLEX, 1.0, 1, &baseLine);

    int y = (rgb.rows - label_size.height) / 2;
    int x = (rgb.cols - label_size.width) / 2;

    cv::rectangle(rgb, cv::Rect(cv::Point(x, y),
                                cv::Size(label_size.width, label_size.height + baseLine)),
                  cv::Scalar(255, 255, 255), -1);

    cv::putText(rgb, text, cv::Point(x, y + label_size.height),
                cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 0, 0));

    return 0;
}

float avg_fps = 0.f;

static int draw_fps(cv::Mat &rgb) {
    // resolve moving average

    {
        static double t0 = 0.f;
        static float fps_history[10] = {0.f};

        double t1 = ncnn::get_current_time();
        if (t0 == 0.f) {
            t0 = t1;
            return 0;
        }

        float fps = 1000.f / (t1 - t0);
        t0 = t1;

        for (int i = 9; i >= 1; i--) {
            fps_history[i] = fps_history[i - 1];
        }
        fps_history[0] = fps;

        if (fps_history[9] == 0.f) {
            return 0;
        }

        for (int i = 0; i < 10; i++) {
            avg_fps += fps_history[i];
        }
        avg_fps /= 10.f;
    }

    char text[32];
    sprintf(text, "FPS=%.2f", avg_fps);

    int baseLine = 0;
    cv::Size label_size = cv::getTextSize(text, cv::FONT_HERSHEY_SIMPLEX, 0.5, 1, &baseLine);

    int y = 0;
    int x = rgb.cols - label_size.width;

    cv::rectangle(rgb, cv::Rect(cv::Point(x, y),
                                cv::Size(label_size.width, label_size.height + baseLine)),
                  cv::Scalar(255, 255, 255), -1);

    cv::putText(rgb, text, cv::Point(x, y + label_size.height),
                cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 0));

    return 0;
}

static Yolo *g_yolo = 0;
static ncnn::Mutex lock;
std::vector<Object> objects;

class MyNdkCamera : public NdkCameraWindow {
public:
    virtual void on_image_render(cv::Mat &rgb) const;
};

void MyNdkCamera::on_image_render(cv::Mat &rgb) const {
    // nanodet
    {
        ncnn::MutexLockGuard g(lock);

        if (g_yolo) {
            g_yolo->detect(rgb, objects);
            g_yolo->draw(rgb, objects);
            //TODO 这里的数据是否可以吐出给java层使用
//            __android_log_print(ANDROID_LOG_DEBUG, "ncnn-log", "objects: %d", objects.size());
//            if(objects.size() > 0){
//                __android_log_print(ANDROID_LOG_DEBUG, "ncnn-log", "objects.label: %d %lf ", objects[0].label,objects[0].prob);
//                __android_log_print(ANDROID_LOG_DEBUG, "ncnn-log", "objects.rect(x,y,w,h): %f %lf %f %lf", objects[0].rect.x,objects[0].rect.y,objects[0].rect.width,objects[0].rect.height);
//
//            }
        } else {
            draw_unsupported(rgb);
        }
    }

    draw_fps(rgb);
}

static MyNdkCamera *g_camera = 0;

static jclass yoloJavaCls = NULL;
static jmethodID constructortorId;
static jfieldID xId;
static jfieldID yId;
static jfieldID wId;
static jfieldID hId;
static jfieldID labelId;
static jfieldID probId;
static jfieldID curFpsId;
static jfieldID nameId;
// 先定义返回的数据
std::vector<char*> cppStrings;

extern "C" {

JNIEXPORT jint JNI_OnLoad(JavaVM *vm, void *reserved) {
    __android_log_print(ANDROID_LOG_DEBUG, "ncnn", "JNI_OnLoad");

    g_camera = new MyNdkCamera;
    return JNI_VERSION_1_4;
}

JNIEXPORT void JNI_OnUnload(JavaVM *vm, void *reserved) {
    __android_log_print(ANDROID_LOG_DEBUG, "ncnn", "JNI_OnUnload");

    {
        ncnn::MutexLockGuard g(lock);

        delete g_yolo;
        g_yolo = 0;
    }

    delete g_camera;
    g_camera = 0;
}

// public native boolean loadModel(AssetManager mgr, int modelid, int cpugpu);
JNIEXPORT jboolean JNICALL
Java_com_tencent_yolov8ncnn_Yolov8Ncnn_loadModel(JNIEnv *env, jobject thiz, jobject assetManager,
                                                 jstring modelName, jint cpugpu,jobjectArray stuffNames) {
    if (cpugpu < 0 || cpugpu > 1) {
        return JNI_FALSE;
    }
//  注意需要YoloAPI.Obj是在java层定义的，为了使用它返回结果，需要将它转换为C++层的jclass格式
    jclass yoloModelCls = env->FindClass("com/lianyun/perry/cback/YoloModel");   // 注意名称要对应
    yoloJavaCls = reinterpret_cast<jclass>(env->NewGlobalRef(yoloModelCls));
    constructortorId = env->GetMethodID(yoloJavaCls, "<init>", "()V");  // 注意名称要对应
    xId = env->GetFieldID(yoloJavaCls, "x", "F");
    yId = env->GetFieldID(yoloJavaCls, "y", "F");
    wId = env->GetFieldID(yoloJavaCls, "w", "F");
    hId = env->GetFieldID(yoloJavaCls, "h", "F");
    labelId = env->GetFieldID(yoloJavaCls, "label", "I");
    probId = env->GetFieldID(yoloJavaCls, "prob", "F");
    curFpsId = env->GetFieldID(yoloJavaCls, "curFps", "F");
    nameId = env->GetFieldID(yoloJavaCls, "name", "Ljava/lang/String;");
    AAssetManager *mgr = AAssetManager_fromJava(env, assetManager);

    __android_log_print(ANDROID_LOG_DEBUG, "ncnn", "loadModel %p", mgr);

    const int target_sizes[] =
            {
                    320,
                    320,
            };

    const float mean_vals[][3] =
            {
                    {103.53f, 116.28f, 123.675f},
                    {103.53f, 116.28f, 123.675f},
            };

    const float norm_vals[][3] =
            {
                    {1 / 255.f, 1 / 255.f, 1 / 255.f},
                    {1 / 255.f, 1 / 255.f, 1 / 255.f},
            };

    const char *modeltype = env->GetStringUTFChars(modelName, nullptr);
    int target_size = target_sizes[0];
    bool use_gpu = (int) cpugpu == 1;

    // reload
    {
        ncnn::MutexLockGuard g(lock);

        if (use_gpu && ncnn::get_gpu_count() == 0) {
            // no gpu
            delete g_yolo;
            g_yolo = 0;
        } else {
            if (!g_yolo) {
                g_yolo = new Yolo;
            }
            jsize arrayLength = env->GetArrayLength(stuffNames);
            cppStrings = std::vector<char*>(arrayLength);
            for (jsize i = 0; i < arrayLength; i++) {
                jobject element = env->GetObjectArrayElement(stuffNames, i);
//                if (element != nullptr && env->IsInstanceOf(element, env->FindClass("java/lang/String"))) {
                    // 确保元素是 jstring 类型
                    jstring jstr = static_cast<jstring>(element);
                    const char* cstr = env->GetStringUTFChars(jstr, nullptr);
                    cppStrings[i] = const_cast<char*>(cstr); // 转换为 char*
//                __android_log_print(ANDROID_LOG_DEBUG, "ncnn_log", "cstr %s", cstr);
//                    env->ReleaseStringUTFChars(jstr, cstr);
//                }else{
                    // 处理非字符串元素的情况
//                    constCharArray[i] = nullptr;
//                }
            }
            g_yolo->load(mgr, modeltype, target_size, mean_vals[0],norm_vals[0], cppStrings,use_gpu);
        }
    }

    return JNI_TRUE;
}

// public native boolean openCamera(int facing);
JNIEXPORT jboolean JNICALL
Java_com_tencent_yolov8ncnn_Yolov8Ncnn_openCamera(JNIEnv *env, jobject thiz, jint facing) {
    if (facing < 0 || facing > 1)
        return JNI_FALSE;

    __android_log_print(ANDROID_LOG_DEBUG, "ncnn", "openCamera %d", facing);

    g_camera->open((int) facing);

    return JNI_TRUE;
}

// public native boolean closeCamera();
JNIEXPORT jboolean JNICALL
Java_com_tencent_yolov8ncnn_Yolov8Ncnn_closeCamera(JNIEnv *env, jobject thiz) {
    __android_log_print(ANDROID_LOG_DEBUG, "ncnn", "closeCamera");

    g_camera->close();

    return JNI_TRUE;
}

// public native boolean setOutputWindow(Surface surface);
JNIEXPORT jboolean JNICALL
Java_com_tencent_yolov8ncnn_Yolov8Ncnn_setOutputWindow(JNIEnv *env, jobject thiz, jobject surface) {
    ANativeWindow *win = ANativeWindow_fromSurface(env, surface);

    __android_log_print(ANDROID_LOG_DEBUG, "ncnn", "setOutputWindow %p", win);

    g_camera->set_window(win);

    return JNI_TRUE;
}

JNIEXPORT jobjectArray JNICALL
Java_com_tencent_yolov8ncnn_Yolov8Ncnn_getSeeStuff(JNIEnv *env, jobject thiz) {
    // 在detectPicture方法中将结果保存在了 objects 中，还需继续对他进行转换
    jobjectArray jObjArray = env->NewObjectArray(objects.size(), yoloJavaCls, NULL);
    jobject jObj = env->NewObject(yoloJavaCls, constructortorId, thiz);

    for (size_t i = 0; i < objects.size(); i++) {
        env->SetFloatField(jObj, xId, objects[i].rect.x);
        env->SetFloatField(jObj, yId, objects[i].rect.y);
        env->SetFloatField(jObj, wId, objects[i].rect.width);
        env->SetFloatField(jObj, hId, objects[i].rect.height);
        env->SetIntField(jObj, labelId, objects[i].label);
        env->SetFloatField(jObj, probId, objects[i].prob);
        env->SetFloatField(jObj, curFpsId, avg_fps);
        jstring jName = env->NewStringUTF(cppStrings[i]);
        env->SetObjectField(jObj,nameId,jName);
        env->SetObjectArrayElement(jObjArray, i, jObj);
    }
    return jObjArray;
}
}