#pragma once

#include <iostream>
#include <assert.h>
#include <vector>
#include <unordered_map>
#include <string>

// ����һ�������н�������
class CommandLineParser
{
public:
	// ����һ���ṹ�壬�����洢������ѡ�����Ϣ
	struct CommandLineOption {
		std::vector<std::string> commands;	// ��ѡ��������б�����"-h"��"--help"
		std::string value;					// ��ѡ���ֵ������еĻ�
		bool hasValue = false;				// ��ѡ���Ƿ���Ҫһ��ֵ
		std::string help;					// ��ѡ��İ�����Ϣ
		bool set = false;					// ��ѡ���Ƿ�����
	};
	// ����һ����ϣ�������洢���е�������ѡ���ѡ�������Ϊ��
	std::unordered_map<std::string, CommandLineOption> options;

	// ����һ���������������һ��������ѡ��
	void add(std::string name, std::vector<std::string> commands, bool hasValue, std::string help)
	{
		options[name].commands = commands;	// ���ø�ѡ��������б�
		options[name].help = help;			// ���ø�ѡ��İ�����Ϣ
		options[name].set = false;			// ��ʼ����ѡ��Ϊδ����
		options[name].hasValue = hasValue;	// ���ø�ѡ���Ƿ���Ҫһ��ֵ
		options[name].value = "";			// ��ʼ����ѡ���ֵΪ���ַ���
	}

	// ����һ��������������ӡ���е�������ѡ��İ�����Ϣ
	void printHelp()
	{
		std::cout << "Available command line options:\n";	// �����ʾ��Ϣ
		for (auto option : options) {						// �������е�������ѡ��
			std::cout << " ";								// ����ո�
			for (size_t i = 0; i < option.second.commands.size(); i++) {	// ������ѡ��������б�
				std::cout << option.second.commands[i];		// ���������
				if (i < option.second.commands.size() - 1) {// ����������һ������
					std::cout << ", ";						// ������źͿո�
				}
			}
			std::cout << ": " << option.second.help << "\n";// ���ð�ţ��ո�͸�ѡ��İ�����Ϣ
		}
		std::cout << "Press any key to close...";			// �����ʾ��Ϣ
	}

	// ����һ����������������һ���ַ����������͵Ĳ����б�
	void parse(std::vector<const char*> arguments)
	{
		bool printHelp = false; // ����һ��������������������Ƿ���Ҫ��ӡ������Ϣ
		for (auto& option : options) { // �������е�������ѡ��
			for (auto& command : option.second.commands) { // ������ѡ��������б�
				for (size_t i = 0; i < arguments.size(); i++) { // ���������б�
					if (strcmp(arguments[i], command.c_str()) == 0) { // ���������������ͬ
						option.second.set = true; // ���ø�ѡ��Ϊ������
						if (option.second.hasValue) { // �����ѡ����Ҫһ��ֵ
							if (arguments.size() > i + 1) { // ��������б�����һ��Ԫ��
								option.second.value = arguments[i + 1]; // ���ø�ѡ���ֵΪ��һ������
							}
							if (option.second.value == "") { // �����ѡ���ֵΪ���ַ���
								printHelp = true; // ���ô�ӡ������ϢΪ��
								break; // ����ѭ��
							}
						}
					}
				}
			}
		}
		if (printHelp) { // �����Ҫ��ӡ������Ϣ
			options["help"].set = true; // ���ð���ѡ��Ϊ������
		}
	}

	// ����һ����������������һ���ַ�ָ���������͵Ĳ����б�
	void parse(int argc, char* argv[])
	{
		std::vector<const char*> args; // ����һ���ַ��������������洢�����б�
		for (int i = 0; i < argc; i++) { // ������������
			args.push_back(argv[i]); // ��������ӵ��ַ���������
		};
		parse(args); // �������涨��ķ����������ַ����������͵Ĳ����б�
	}

	// ����һ�������������ж�һ��ѡ���Ƿ�����
	bool isSet(std::string name)
	{
		return ((options.find(name) != options.end()) && options[name].set); // ���ظ�ѡ���Ƿ�����ڹ�ϣ���У������Ƿ�����
	}

	// ����һ��������������ȡһ��ѡ���ֵ�����ַ������ͷ��أ����û��ֵ������Ĭ��ֵ
	std::string getValueAsString(std::string name, std::string defaultValue)
	{
		assert(options.find(name) != options.end()); // ���Ը�ѡ������ڹ�ϣ����
		std::string value = options[name].value; // ��ȡ��ѡ���ֵ
		return (value != "") ? value : defaultValue; // ���ֵ��Ϊ���ַ���������ֵ�����򷵻�Ĭ��ֵ
	}

	// ����һ��������������ȡһ��ѡ���ֵ�����������ͷ��أ����û��ֵ������Ĭ��ֵ
	int32_t getValueAsInt(std::string name, int32_t defaultValue)
	{
		assert(options.find(name) != options.end()); // ���Ը�ѡ������ڹ�ϣ����
		std::string value = options[name].value; // ��ȡ��ѡ���ֵ
		if (value != "") { // ���ֵ��Ϊ���ַ���
			char* numConvPtr; // ����һ���ַ�ָ�룬�����洢ת��������ֵĵ�ַ
			int32_t intVal = strtol(value.c_str(), &numConvPtr, 10); // ���ַ������͵�ֵת��Ϊʮ���Ƶ���������
			return (intVal > 0) ? intVal : defaultValue; // ���ת��������������㣬���ظ����������򷵻�Ĭ��ֵ
		}
		else { // ���ֵΪ���ַ���
			return defaultValue; // ����Ĭ��ֵ
		}
		return int32_t(); // ����һ���յ���������
	}

};
