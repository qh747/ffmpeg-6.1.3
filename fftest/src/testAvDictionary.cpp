extern "C" {
#include <libavutil/log.h>
#include <libavutil/dict.h>
}

#include "testAvDictionary.h"

namespace TEST_AV_DICTIONARY { 

// 测试字典基本功能
void TestDictionaryBasic() {
    // 创建字典
    AVDictionary* dict = nullptr;

    // 添加键值对
    av_dict_set(&dict, "hello", "world", 0);

    // 获取键值对
    AVDictionaryEntry* entry = av_dict_get(dict, "hello", nullptr, 0);
    if (nullptr != entry) {
        av_log(nullptr, AV_LOG_INFO, "Key: %s. Value: %s\n", entry->key, entry->value);
    }

    // 销毁字典
    av_dict_free(&dict);
}

// 测试字典遍历功能
void TestDictionaryForeach() {
    // 创建字典
    AVDictionary* dict = nullptr;

    // 添加键值对
    av_dict_set(&dict, "hello", "world", 0);
    av_dict_set(&dict, "hi"   , "world", 0);
    av_dict_set(&dict, "bye"  , "world", 0);

    // 获取键值对数量
    int count = av_dict_count(dict);
    av_log(nullptr, AV_LOG_INFO, "Key-value count: %d\n", count);

    // 遍历字典
    AVDictionaryEntry* entry = nullptr;
    while (true) {
        entry = av_dict_get(dict, "", entry, AV_DICT_IGNORE_SUFFIX);
        if (nullptr == entry) {
            break;
        }

        av_log(nullptr, AV_LOG_INFO, "Key: %s. Value: %s\n", entry->key, entry->value);
    }

    // 销毁字典
    av_dict_free(&dict);
}

}; // namespace TEST_AV_DICTIONARY