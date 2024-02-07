#pragma once

#include <iostream>
#include <assert.h>
#include <vector>
#include <unordered_map>
#include <string>

// 定义一个命令行解析器类
class CommandLineParser
{
public:
	// 定义一个结构体，用来存储命令行选项的信息
	struct CommandLineOption {
		std::vector<std::string> commands;	// 该选项的命令列表，例如"-h"或"--help"
		std::string value;					// 该选项的值，如果有的话
		bool hasValue = false;				// 该选项是否需要一个值
		std::string help;					// 该选项的帮助信息
		bool set = false;					// 该选项是否被设置
	};
	// 定义一个哈希表，用来存储所有的命令行选项，以选项的名称为键
	std::unordered_map<std::string, CommandLineOption> options;

	// 定义一个方法，用来添加一个命令行选项
	void add(std::string name, std::vector<std::string> commands, bool hasValue, std::string help)
	{
		options[name].commands = commands;	// 设置该选项的命令列表
		options[name].help = help;			// 设置该选项的帮助信息
		options[name].set = false;			// 初始化该选项为未设置
		options[name].hasValue = hasValue;	// 设置该选项是否需要一个值
		options[name].value = "";			// 初始化该选项的值为空字符串
	}

	// 定义一个方法，用来打印所有的命令行选项的帮助信息
	void printHelp()
	{
		std::cout << "Available command line options:\n";	// 输出提示信息
		for (auto option : options) {						// 遍历所有的命令行选项
			std::cout << " ";								// 输出空格
			for (size_t i = 0; i < option.second.commands.size(); i++) {	// 遍历该选项的命令列表
				std::cout << option.second.commands[i];		// 输出该命令
				if (i < option.second.commands.size() - 1) {// 如果不是最后一个命令
					std::cout << ", ";						// 输出逗号和空格
				}
			}
			std::cout << ": " << option.second.help << "\n";// 输出冒号，空格和该选项的帮助信息
		}
		std::cout << "Press any key to close...";			// 输出提示信息
	}

	// 定义一个方法，用来解析一个字符串向量类型的参数列表
	void parse(std::vector<const char*> arguments)
	{
		bool printHelp = false; // 定义一个布尔变量，用来标记是否需要打印帮助信息
		for (auto& option : options) { // 遍历所有的命令行选项
			for (auto& command : option.second.commands) { // 遍历该选项的命令列表
				for (size_t i = 0; i < arguments.size(); i++) { // 遍历参数列表
					if (strcmp(arguments[i], command.c_str()) == 0) { // 如果参数和命令相同
						option.second.set = true; // 设置该选项为已设置
						if (option.second.hasValue) { // 如果该选项需要一个值
							if (arguments.size() > i + 1) { // 如果参数列表还有下一个元素
								option.second.value = arguments[i + 1]; // 设置该选项的值为下一个参数
							}
							if (option.second.value == "") { // 如果该选项的值为空字符串
								printHelp = true; // 设置打印帮助信息为真
								break; // 跳出循环
							}
						}
					}
				}
			}
		}
		if (printHelp) { // 如果需要打印帮助信息
			options["help"].set = true; // 设置帮助选项为已设置
		}
	}

	// 定义一个方法，用来解析一个字符指针数组类型的参数列表
	void parse(int argc, char* argv[])
	{
		std::vector<const char*> args; // 定义一个字符串向量，用来存储参数列表
		for (int i = 0; i < argc; i++) { // 遍历参数个数
			args.push_back(argv[i]); // 将参数添加到字符串向量中
		};
		parse(args); // 调用上面定义的方法，解析字符串向量类型的参数列表
	}

	// 定义一个方法，用来判断一个选项是否被设置
	bool isSet(std::string name)
	{
		return ((options.find(name) != options.end()) && options[name].set); // 返回该选项是否存在于哈希表中，并且是否被设置
	}

	// 定义一个方法，用来获取一个选项的值，以字符串类型返回，如果没有值，返回默认值
	std::string getValueAsString(std::string name, std::string defaultValue)
	{
		assert(options.find(name) != options.end()); // 断言该选项存在于哈希表中
		std::string value = options[name].value; // 获取该选项的值
		return (value != "") ? value : defaultValue; // 如果值不为空字符串，返回值，否则返回默认值
	}

	// 定义一个方法，用来获取一个选项的值，以整数类型返回，如果没有值，返回默认值
	int32_t getValueAsInt(std::string name, int32_t defaultValue)
	{
		assert(options.find(name) != options.end()); // 断言该选项存在于哈希表中
		std::string value = options[name].value; // 获取该选项的值
		if (value != "") { // 如果值不为空字符串
			char* numConvPtr; // 定义一个字符指针，用来存储转换后的数字的地址
			int32_t intVal = strtol(value.c_str(), &numConvPtr, 10); // 将字符串类型的值转换为十进制的整数类型
			return (intVal > 0) ? intVal : defaultValue; // 如果转换后的整数大于零，返回该整数，否则返回默认值
		}
		else { // 如果值为空字符串
			return defaultValue; // 返回默认值
		}
		return int32_t(); // 返回一个空的整数类型
	}

};
