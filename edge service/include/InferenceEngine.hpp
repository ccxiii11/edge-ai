#pragma once
#include"Batch.hpp"
#include<NvInfer.h>
#include<NvOnnxParser.h>
#include<iostream>
#include<string>
#include<vector>
#include<cstddef>
class InferenceEngine
{
	public:
	 	
		explicit  InferenceEngine(size_t profile_count);
		~InferenceEngine();
		nvinfer1::IExecutionContext*createContext(int profile_index);
		std::vector<std::string> infer(
				nvinfer1::IExecutionContext *context,
				const Batch& batch,
				int profile_index
				);
	private:
		class Logger:public nvinfer1::ILogger
	{
		public :
			void log
				(Severity severity,
				const char*msg 
				 )noexcept override
				{
					std::cout<<msg<<std::endl;
				}
	};
		private:
		Logger logger_;
		nvinfer1::IBuilder*builder_=nullptr;
		nvinfer1::INetworkDefinition*network_=nullptr;
		nvinfer1::ICudaEngine*engine_ =nullptr;
		
};
