#pragma once

namespace Cetus {

	class Layer
	{
	public:
		// ����һ����������������������������д����ʵ�ֶ�̬
		virtual ~Layer() = default;

		// ����һ���麯������������������д����ʵ����ͼ�㱻���ʱִ��һЩ����
		virtual void OnAttach() {}
		// ����һ���麯������������������д����ʵ����ͼ�㱻�Ƴ�ʱִ��һЩ����
		virtual void OnDetach() {}

		// ����һ���麯������������������д����ʵ����ÿһ֡����ͼ���״̬
		virtual void OnUpdate(float ts) {}
		// ����һ���麯������������������д����ʵ����ÿһ֡��Ⱦͼ����û�����
		virtual void OnUIRender() {}
	};

}