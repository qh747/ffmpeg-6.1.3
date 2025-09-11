#pragma once 

namespace TEST_AV_FORMAT {

// 测试输出文件的媒体流信息
void TestShowStreamInfo();

// 测试输出文件的前100条帧信息
void TestShowFstHundredFrames();

// 测试将mp4文件重新解封装/封装为flv文件
void TestRemuxMp4ToFlv();

}; // namespace TEST_AV_FORMAT