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

package com.tencent.yolov8ncnn;

import android.content.res.AssetManager;
import android.view.Surface;

import com.lianyun.perry.cback.YoloModel;

public class Yolov8Ncnn
{
    public native boolean loadModel(AssetManager mgr, String modelName, int cpugpu);
    public native boolean openCamera(int facing);
    public native boolean closeCamera();
    public native boolean setOutputWindow(Surface surface);

    /**
     * 获取当前摄像头显示页面上所识别出来的物品
     * 在java中调用可以使用线程轮训
     * 例如：
     *  @Override
     *         public void run() {
     *             do{
     *                 try {
     *                     Thread.sleep(200);
     *                 } catch (InterruptedException e) {
     *                     throw new RuntimeException(e);
     *                 }
     *                 if(mPauseThread){
     *                     Log.d(TAG,"YoloThread 线程暂停了");
     *                     continue;
     *                 }
     *                 YoloModel[] yoloModels = yolov8ncnn.getSeeStuff();
     *                 Log.d(TAG,"yoloModels.length = "+yoloModels.length);
     *                 if(yoloModels.length > 0){
     *                     for(int i = 0;i<yoloModels.length;i++){
     *                         Log.d(TAG,"yoloModel [" + i + "]: "+yoloModels[i]);
     *                     }
     *                 }
     *             }while (!mStopThread);
     *             Log.d(TAG, "执行线程结束了");
     *         }
     * @return
     */
    public native YoloModel[] getSeeStuff();


    static {
        System.loadLibrary("yolov8ncnn");
    }
}
