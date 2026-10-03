#include "InferenceEngine.hpp"
#include <cuda_runtime_api.h>
#include <iostream>

InferenceEngine::InferenceEngine(size_t profile_count)
{
    builder_ =
        nvinfer1::createInferBuilder(logger_);

    if (builder_ == nullptr)
    {
        std::cout
            << "Failed to create TensorRT builder."
            << std::endl;

        return;
    }

    std::cout
        << "TensorRT Builder created."
        << std::endl;


    uint32_t network_flags =
        1U << static_cast<uint32_t>(
            nvinfer1::NetworkDefinitionCreationFlag::kEXPLICIT_BATCH
        );

    network_ =
        builder_->createNetworkV2(network_flags);

    if (network_ == nullptr)
    {
        std::cout
            << "Failed to create TensorRT network."
            << std::endl;

        return;
    }

    std::cout
        << "TensorRT Network created."
        << std::endl;


    nvonnxparser::IParser *parser =
	    nvonnxparser::createParser(
			    *network_,
			    logger_
			    );
    if(parser ==nullptr)
    {
	    std::cout
		    <<"Failed to create ONNX parser."
		    <<std::endl;
	    return ;
    }
    std::cout
	    <<"ONNX parser created"
	    <<std::endl;
    bool parsed =parser->parseFromFile(
		    "/home/ccc/ai_infra/mnist_dynamic.onnx",
		    static_cast<int>(
			    nvinfer1::ILogger::Severity::kWARNING
			    )
		    );
    if(!parsed)
    {
	    std::cout
		    <<"Failed to parse ONNX model"
		    <<std::endl;

	    parser->destroy();
	    return ;

    }
    std::cout
	    <<"ONNX model parsed successfully"
	    << std::endl;
    
    int input_count = network_->getNbInputs();

std::cout
    << "Network input count = "
    << input_count
    << std::endl;

for (int i = 0; i < input_count; ++i)
{
    nvinfer1::ITensor* input =
        network_->getInput(i);

    std::cout
        << "Input "
        << i
        << ": name = "
        << input->getName()
        << ", dims = "
        << input->getDimensions().nbDims
        << std::endl;

    for (int j = 0;
         j < input->getDimensions().nbDims;
         ++j)
    {
        std::cout
            << "  dim["
            << j
            << "] = "
            << input->getDimensions().d[j]
            << std::endl;
    }
}

    nvinfer1::IBuilderConfig* config =
        builder_->createBuilderConfig();

    if (config == nullptr)
    {
        std::cout
            << "Failed to create builder config."
            << std::endl;

        return;
    }


    for(size_t i=0;i<profile_count;++i){
    nvinfer1::IOptimizationProfile* profile =
        builder_->createOptimizationProfile();

    if (profile == nullptr)
    {
        std::cout
            << "Failed to create optimization profile."
            << std::endl;

        return;
    }


    profile->setDimensions(
        "Input3",
        nvinfer1::OptProfileSelector::kMIN,
        nvinfer1::Dims4(
            1,
            1,
            28,
            28
        )
    );


    profile->setDimensions(
        "Input3",
        nvinfer1::OptProfileSelector::kOPT,
        nvinfer1::Dims4(
            4,
            1,
            28,
            28
        )
    );


    profile->setDimensions(
        "Input3",
        nvinfer1::OptProfileSelector::kMAX,
        nvinfer1::Dims4(
            8,
            1,
            28,
            28
        )
    );


    config->addOptimizationProfile(profile);
    }
    engine_ =
        builder_->buildEngineWithConfig(
            *network_,
            *config
        );

    if (engine_ == nullptr)
    {
        std::cout
            << "Failed to build TensorRT engine."
            << std::endl;

        return;
    }

    std::cout
        << "TensorRT Engine built."
        << std::endl;
}




nvinfer1::IExecutionContext*
InferenceEngine::createContext(int profile_index)
{
    if (engine_ == nullptr)
    {
        std::cout
            << "Engine is not available."
            << std::endl;

        return nullptr;
    }

    std::cout
    << "Engine has "
    << engine_->getNbOptimizationProfiles()
    << " optimization profiles."
    << std::endl;
    if(profile_index<0)
    {
	    std::cout<<"invalid profile index."
		    <<std::endl;
	    return nullptr;
    }

    nvinfer1::IExecutionContext* context =
        engine_->createExecutionContext();

    if (context == nullptr)
    {
        std::cout
            << "Failed to create Execution Context."
            << std::endl;

        return nullptr;
    }

    bool profile_ok=
	    context->setOptimizationProfile(profile_index);
    if(!profile_ok)
    {
	    std::cout
		    <<"Failed to set optimization profile"
		    <<std::endl;
	    context->destroy();
	    return nullptr;
    }
    std::cout
        << "Execution Context created,profile="
	<<profile_index
        << std::endl;

    return context;
}

InferenceEngine::~InferenceEngine()
{
    
    if (engine_ != nullptr)
    {
        engine_->destroy();
        engine_ = nullptr;
    }

    if (network_ != nullptr)
    {
        network_->destroy();
        network_ = nullptr;
    }

    if (builder_ != nullptr)
    {
        builder_->destroy();
        builder_ = nullptr;
    }
}


std::vector<std::string> InferenceEngine::infer(
    nvinfer1::IExecutionContext* context,
    const Batch& batch,
    int profile_index)
{
    if (context == nullptr)
    {
        std::cout
            << "Invalid Execution Context."
            << std::endl;

        return {};
    }

    const int batch_size =
        static_cast<int>(batch.tasks.size());

    if (batch_size < 1 || batch_size > 8)
    {
        std::cout
            << "Invalid batch size: "
            << batch_size
            << std::endl;

        return {};
    }

    const int elements_per_sample =
        1 * 28 * 28;

    const int total_elements =
        batch_size * elements_per_sample;

    const int output_elements_per_sample = 10;

    const int total_output_elements =
        batch_size * output_elements_per_sample;

    std::vector<float> input_data(
        total_elements
    );

    std::vector<float> output_data(
        total_output_elements
    );

    // 准备输入数据
    for (int i = 0; i < total_elements; ++i)
    {
        input_data[i] =
            static_cast<float>((i % 20) - 10);
    }

    float* device_input = nullptr;
    float* device_output = nullptr;

    cudaMalloc(
        reinterpret_cast<void**>(&device_input),
        total_elements * sizeof(float)
    );

    cudaMalloc(
        reinterpret_cast<void**>(&device_output),
        total_output_elements * sizeof(float)
    );

    cudaMemcpy(
        device_input,
        input_data.data(),
        total_elements * sizeof(float),
        cudaMemcpyHostToDevice
    );

    int input_index =
        profile_index * 2;

    int output_index =
        profile_index * 2 + 1;

    bool dimensions_ok =
        context->setBindingDimensions(
            input_index,
            nvinfer1::Dims4(
                batch_size,
                1,
                28,
                28
            )
        );

    if (!dimensions_ok)
    {
        std::cout
            << "Failed to set binding dimensions."
            << std::endl;

        cudaFree(device_input);
        cudaFree(device_output);

        return {};
    }

    void* bindings[6] = {};

    bindings[input_index] =
        device_input;

    bindings[output_index] =
        device_output;

    bool success =
        context->enqueueV2(
            bindings,
            0,
            nullptr
        );

    if (!success)
    {
        std::cout
            << "TensorRT enqueueV2 failed."
            << std::endl;

        cudaFree(device_input);
        cudaFree(device_output);

        return {};
    }

    cudaDeviceSynchronize();

    cudaMemcpy(
        output_data.data(),
        device_output,
        total_output_elements * sizeof(float),
        cudaMemcpyDeviceToHost
    );

    std::vector<std::string> results;

    results.reserve(batch.tasks.size());

    for (int i = 0; i < batch_size; ++i)
    {
        int index =
            i * output_elements_per_sample;

        std::string result =
            "AI result: " +
            std::to_string(output_data[index]);

        results.push_back(result);
    }

    cudaFree(device_input);
    cudaFree(device_output);

    return results;
}
