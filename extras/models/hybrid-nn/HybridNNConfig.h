#ifndef ANIRA_HYBRIDNNCONFIG_H
#define ANIRA_HYBRIDNNCONFIG_H

#include <anira/anira.h>

#if WIN32
#define HYBRIDNN_MAX_INFERENCE_TIME 16384
#else
#define HYBRIDNN_MAX_INFERENCE_TIME 256
#endif

static anira::InferenceConfig hybridNNConfig(
#if defined(USE_LIBTORCH) || defined(MODEL_CONFIG_DEBUG)
        GUITARLSTM_MODELS_PATH_PYTORCH + std::string("model_0/model_0-streaming.pt"),
        {128, 1, 150},
        {128, 1},
#endif
#if defined(USE_ONNXRUNTIME) || defined(MODEL_CONFIG_DEBUG)
        GUITARLSTM_MODELS_PATH_TENSORFLOW + std::string("model_0/model_0-tflite-streaming.onnx"),
        {128, 150, 1},
        {128, 1},
#endif
#if defined(USE_TFLITE) || defined(MODEL_CONFIG_DEBUG)
        GUITARLSTM_MODELS_PATH_TENSORFLOW + std::string("model_0/model_0-streaming.tflite"),
        {128, 150, 1},
        {128, 1},
#endif
        128,
        1,
        150,
        1,
        HYBRIDNN_MAX_INFERENCE_TIME,
        0,
        false,
        0.5f,
        false
);

#endif //ANIRA_HYBRIDNNCONFIG_H
