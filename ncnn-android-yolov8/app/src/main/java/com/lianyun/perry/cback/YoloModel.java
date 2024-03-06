package com.lianyun.perry.cback;

import android.util.Log;

public class YoloModel {
    public float x;
    public float y;
    public float w;
    public float h;
    public int label;
    public float prob;

    public String name;

    public float curFps;


    @Override
    public String toString() {
        return "YoloModel{" +
                "x=" + x +
                ", y=" + y +
                ", w=" + w +
                ", h=" + h +
                ", label=" + label +
                ", prob=" + prob +
                ", name=" + name +
                ", curFps=" + curFps +
                '}';
    }
}
