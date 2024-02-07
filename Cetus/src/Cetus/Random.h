#pragma once
// 使用#pragma once指令，防止头文件被重复包含 #pragma once
#include <random>

#include <glm/glm.hpp>

namespace Cetus {

	class Random
	{
	public:
		static void Init()		
		{// 定义一个静态函数，用于初始化随机数引擎，使用std::random_device作为种子
			s_RandomEngine.seed(std::random_device()());
		}

		static uint32_t UInt()	
		{// 定义一个静态函数，用于生成一个无符号整数类型的随机数，使用s_Distribution作为分布
			return s_Distribution(s_RandomEngine);
		}

		static uint32_t UInt(uint32_t min, uint32_t max)
		{// 定义一个静态函数，用于生成一个在[min, max]范围内的无符号整数类型的随机数，使用s_Distribution作为分布，并对结果取模，余数部分为{0,..,max-min}
			return min + (s_Distribution(s_RandomEngine) % (max - min + 1));
		}

		static float Float()
		{// 定义一个静态函数，用于生成一个浮点类型的随机数，使用s_Distribution作为分布，并除以uint32_t的最大值，使结果在[0, 1]范围内
			return (float)s_Distribution(s_RandomEngine) / (float)std::numeric_limits<uint32_t>::max();
		}

		static glm::vec3 Vec3()
		{// 定义一个静态函数，用于生成一个三维向量类型的随机数，使用Float()函数生成每个分量
			return glm::vec3(Float(), Float(), Float());
		}

		static glm::vec3 Vec3(float min, float max)
		{// 定义一个静态函数，用于生成一个在[min, max]范围内的三维向量类型的随机数，使用Float()函数生成每个分量，并乘以范围差，再加上最小值
			return glm::vec3(Float() * (max - min) + min, Float() * (max - min) + min, Float() * (max - min) + min);
		}

		static glm::vec3 InUnitSphere()
		{// 定义一个静态函数，用于生成一个在单位球内的三维向量类型的随机数，使用Vec3(-1.0f, 1.0f)函数生成一个在[-1, 1]范围内的向量，并对其进行归一化
			return glm::normalize(Vec3(-1.0f, 1.0f));
		}
	private:
		static std::mt19937 s_RandomEngine;// 定义一个静态的随机数引擎，使用std::mt19937作为算法
		// 随机数引擎的名字mt19937是由Mersenne(梅森) Twister算法的特征决定的。
		// Mersenne Twister算法是一种基于Mersenne素数的伪随机数生成算法，它可以产生高质量的32位或64位的无符号整数随机数。
		// Mersenne素数是一种形如2n-1的素数，其中n是一个正整数。
		// mt19937的名字中的19937表示的是它使用的Mersenne素数的指数，即2^(19937-1)是一个Mersenne素数。
		// mt19937还有一个64位版本，叫做mt19937_64，它使用的是219937-1的高64位作为随机数的输出。
		// mt19937和mt19937_64都是C++标准库中的mersenne_twister_engine类的实例化，它们可以在<random>头文件中找到。
		// 定义一个静态的分布，使用std::uniform_int_distribution作为类型，用于生成均匀分布的随机数。
		static std::uniform_int_distribution<std::mt19937::result_type> s_Distribution;
	};

}


