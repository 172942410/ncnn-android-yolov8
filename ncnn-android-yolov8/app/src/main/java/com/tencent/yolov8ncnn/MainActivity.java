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

import android.Manifest;
import android.app.Activity;
import android.content.pm.PackageManager;
import android.graphics.PixelFormat;
import android.os.Bundle;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.WindowManager;
import android.widget.AdapterView;
import android.widget.Button;
import android.widget.Spinner;

import android.support.v4.app.ActivityCompat;
import android.support.v4.content.ContextCompat;

import com.lianyun.perry.cback.YoloModel;

public class MainActivity extends Activity implements SurfaceHolder.Callback
{
    public static final int REQUEST_CAMERA = 100;
    YoloThread yoloThread;
    private Yolov8Ncnn yolov8ncnn = new Yolov8Ncnn();
    private int facing = 0;

    private Spinner spinnerModel;
    private Spinner spinnerCPUGPU;
    private int current_model = 0;
    private String currentModel;
    private int current_cpugpu = 0;

    private SurfaceView cameraView;
    static  String stuffNames[] = {
    "活动扳手",  "对讲机",  "尖嘴钳",  "万用表",  "口笛",  "螺丝刀",  "钥匙",
            "纸盒装",  "视频记录仪",  "扳手",  "卷尺",  "手机",  "双面警示器",  "微波场强仪",
            "毛刷",  "弯弯",  "水壶",  "振电器",  "饮料瓶",  "喷壶",  "液晶显示仪",
            "圆疙瘩",  "36mm呆扳手",  "呆扳手",  "铁棒",  "油壶","包",  "黑垫子",
            "三角套筒扳手",  "灭火器",  "辅助电机", "开箱专用钥匙","黑体",
            "激光瞄准器","14mm呆扳手","6mmL型内六角扳手","8mmL型内六角扳手",
            "T型套筒","十字螺丝刀","对光架","手红旗","磁钢安装尺","铜导线"
};
    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.main);

        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

        cameraView = (SurfaceView) findViewById(R.id.cameraview);

        cameraView.getHolder().setFormat(PixelFormat.RGBA_8888);
        cameraView.getHolder().addCallback(this);

        Button buttonSwitchCamera = (Button) findViewById(R.id.buttonSwitchCamera);
        buttonSwitchCamera.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View arg0) {

                int new_facing = 1 - facing;

                yolov8ncnn.closeCamera();

                yolov8ncnn.openCamera(new_facing);

                facing = new_facing;
            }
        });
        String [] modelNames = getResources().getStringArray(R.array.model_array);
        currentModel = modelNames[0];
        spinnerModel = (Spinner) findViewById(R.id.spinnerModel);
        spinnerModel.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> arg0, View arg1, int position, long id)
            {
                if (position != current_model)
                {
                    current_model = position;
                    currentModel = modelNames[current_model];
                    reload();
                }
            }

            @Override
            public void onNothingSelected(AdapterView<?> arg0)
            {
            }
        });

        spinnerCPUGPU = (Spinner) findViewById(R.id.spinnerCPUGPU);
        spinnerCPUGPU.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> arg0, View arg1, int position, long id)
            {
                if (position != current_cpugpu)
                {
                    current_cpugpu = position;
                    reload();
                }
            }

            @Override
            public void onNothingSelected(AdapterView<?> arg0)
            {
            }
        });

        reload();
    }

    private void reload()
    {
        boolean ret_init = yolov8ncnn.loadModel(getAssets(), currentModel, current_cpugpu, stuffNames);
        if (!ret_init)
        {
            Log.e("MainActivity", "yolov8ncnn loadModel failed");
        }else{
            //加载成功之后就可以循环回调了
            if(yoloThread == null) {
                yoloThread = new YoloThread();
            }
            if(!yoloThread.isAlive()) {
                mStopThread = false;
                yoloThread.start();
            }
        }
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height)
    {
        yolov8ncnn.setOutputWindow(holder.getSurface());
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder)
    {
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder)
    {
    }

    @Override
    public void onResume()
    {
        super.onResume();

        if (ContextCompat.checkSelfPermission(getApplicationContext(), Manifest.permission.CAMERA) == PackageManager.PERMISSION_DENIED)
        {
            ActivityCompat.requestPermissions(this, new String[] {Manifest.permission.CAMERA}, REQUEST_CAMERA);
        }

        yolov8ncnn.openCamera(facing);
        mPauseThread = false;
    }

    @Override
    public void onPause()
    {
        super.onPause();

        yolov8ncnn.closeCamera();
        mPauseThread = true;
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        mStopThread = true;
    }

    boolean mStopThread = false;
    boolean mPauseThread = false;

    private class YoloThread extends Thread {
        private String TAG = YoloThread.class.getName();
        @Override
        public void run() {
            do{
                try {
                    Thread.sleep(200);
                } catch (InterruptedException e) {
                    throw new RuntimeException(e);
                }
                if(mPauseThread){
                    Log.d(TAG,"YoloThread 线程暂停了");
                    continue;
                }
                YoloModel[] yoloModels = yolov8ncnn.getSeeStuff();
//                Log.d(TAG,"yoloModels.length = "+yoloModels.length);
                if(yoloModels.length > 0){
                    Log.d(TAG,"yoloModels.length = "+yoloModels.length);
                    for(int i = 0;i<yoloModels.length;i++){
                        Log.d(TAG,"yoloModel [" + i + "]: "+yoloModels[i]);
                    }
                }
            }while (!mStopThread);
            Log.d(TAG, "执行线程结束了");
        }
    }
    }
