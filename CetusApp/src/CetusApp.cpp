#include "Cetus/Application.h"
#include "Cetus/EntryPoint.h"

#include "Cetus/Image.h"

class ExampleLayer : public Cetus::Layer
{
public:
	virtual void OnUIRender() override
	{
		ImGui::Begin("Hello");
		if(ImGui::Button("Render")) {
			Render();
		};
		ImGui::End();

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::Begin("Viewport");

		m_ViewportWidth = static_cast<uint32_t>(ImGui::GetContentRegionAvail().x);
		m_ViewportHeight = static_cast<uint32_t>(ImGui::GetContentRegionAvail().y);

		if(m_Image)
			ImGui::Image(m_Image->GetDescriptorSet(), { (float)m_Image->GetWidth(),(float)m_Image->GetHeight() });

		ImGui::End();
		ImGui::PopStyleVar();
	}
	void Render() {
		if (!m_Image || m_ViewportWidth != m_Image->GetWidth() || m_ViewportHeight != m_Image->GetHeight()) {
			m_Image = std::make_shared<Cetus::Image>(m_ViewportWidth, m_ViewportHeight, Cetus::ImageFormat::RGBA);
			delete[] m_ImageData;
			m_ImageData = new uint32_t[m_ViewportWidth * m_ViewportHeight];
		}
		for (uint32_t i = 0; i < m_ViewportWidth * m_ViewportHeight; i++) {
			m_ImageData[i] = 0xffff00ff;
		}
		m_Image->SetData(m_ImageData);
	};
private:
	std::shared_ptr<Cetus::Image> m_Image;
	uint32_t m_ViewportWidth = 0, m_ViewportHeight = 0;
	uint32_t* m_ImageData = nullptr;
};

Cetus::Application* Cetus::CreateApplication(int argc, char** argv)
{
	Cetus::ApplicationSpecification spec;
	spec.Name = "Cetus Example";

	Cetus::Application* app = new Cetus::Application(spec);
	app->PushLayer<ExampleLayer>();
	app->SetMenubarCallback([app]()
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("Exit"))
			{
				app->Close();
			}
			ImGui::EndMenu();
		}
	});
	return app;
}