#pragma once

namespace Cetus {

	class Layer
	{
	public:
		// 定义一个虚析构函数，用于在子类中重写，以实现多态
		virtual ~Layer() = default;

		// 定义一个虚函数，用于在子类中重写，以实现在图层被添加时执行一些操作
		virtual void OnAttach() {}
		// 定义一个虚函数，用于在子类中重写，以实现在图层被移除时执行一些操作
		virtual void OnDetach() {}

		// 定义一个虚函数，用于在子类中重写，以实现在每一帧更新图层的状态
		virtual void OnUpdate(float ts) {}
		// 定义一个虚函数，用于在子类中重写，以实现在每一帧渲染图层的用户界面
		virtual void OnUIRender() {}
	};

}