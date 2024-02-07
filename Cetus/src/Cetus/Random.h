#pragma once
// ʹ��#pragma onceָ���ֹͷ�ļ����ظ����� #pragma once
#include <random>

#include <glm/glm.hpp>

namespace Cetus {

	class Random
	{
	public:
		static void Init()		
		{// ����һ����̬���������ڳ�ʼ����������棬ʹ��std::random_device��Ϊ����
			s_RandomEngine.seed(std::random_device()());
		}

		static uint32_t UInt()	
		{// ����һ����̬��������������һ���޷����������͵��������ʹ��s_Distribution��Ϊ�ֲ�
			return s_Distribution(s_RandomEngine);
		}

		static uint32_t UInt(uint32_t min, uint32_t max)
		{// ����һ����̬��������������һ����[min, max]��Χ�ڵ��޷����������͵��������ʹ��s_Distribution��Ϊ�ֲ������Խ��ȡģ����������Ϊ{0,..,max-min}
			return min + (s_Distribution(s_RandomEngine) % (max - min + 1));
		}

		static float Float()
		{// ����һ����̬��������������һ���������͵��������ʹ��s_Distribution��Ϊ�ֲ���������uint32_t�����ֵ��ʹ�����[0, 1]��Χ��
			return (float)s_Distribution(s_RandomEngine) / (float)std::numeric_limits<uint32_t>::max();
		}

		static glm::vec3 Vec3()
		{// ����һ����̬��������������һ����ά�������͵��������ʹ��Float()��������ÿ������
			return glm::vec3(Float(), Float(), Float());
		}

		static glm::vec3 Vec3(float min, float max)
		{// ����һ����̬��������������һ����[min, max]��Χ�ڵ���ά�������͵��������ʹ��Float()��������ÿ�������������Է�Χ��ټ�����Сֵ
			return glm::vec3(Float() * (max - min) + min, Float() * (max - min) + min, Float() * (max - min) + min);
		}

		static glm::vec3 InUnitSphere()
		{// ����һ����̬��������������һ���ڵ�λ���ڵ���ά�������͵��������ʹ��Vec3(-1.0f, 1.0f)��������һ����[-1, 1]��Χ�ڵ���������������й�һ��
			return glm::normalize(Vec3(-1.0f, 1.0f));
		}
	private:
		static std::mt19937 s_RandomEngine;// ����һ����̬����������棬ʹ��std::mt19937��Ϊ�㷨
		// ��������������mt19937����Mersenne(÷ɭ) Twister�㷨�����������ġ�
		// Mersenne Twister�㷨��һ�ֻ���Mersenne������α����������㷨�������Բ�����������32λ��64λ���޷��������������
		// Mersenne������һ������2n-1������������n��һ����������
		// mt19937�������е�19937��ʾ������ʹ�õ�Mersenne������ָ������2^(19937-1)��һ��Mersenne������
		// mt19937����һ��64λ�汾������mt19937_64����ʹ�õ���219937-1�ĸ�64λ��Ϊ������������
		// mt19937��mt19937_64����C++��׼���е�mersenne_twister_engine���ʵ���������ǿ�����<random>ͷ�ļ����ҵ���
		// ����һ����̬�ķֲ���ʹ��std::uniform_int_distribution��Ϊ���ͣ��������ɾ��ȷֲ����������
		static std::uniform_int_distribution<std::mt19937::result_type> s_Distribution;
	};

}


