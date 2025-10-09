#include "test.h"

#include <imgui/imgui.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <spdlog/spdlog.h>
#include <stb/stb_image.h>

#include "core/assert.h"
#include "core/window.h"

#include "input/keyboard.h"

namespace platformer2d::test {

	CTest::CTest(const int Argc, char* Argv[])
		: CTestBase(Argc, Argv)
	{
	}

	void CTest::Run()
	{
		Running = true;
		const int CatchResult = Catch::Session().run(Args.Argc, Args.Argv);
		LK_DEBUG("Catch result: {}", CatchResult);

		const std::filesystem::path& BinaryDir = GetBinaryDirectory();
		const CWindow& Window = GetWindow();
		const FWindowData& WindowData = Window.GetData();

		CKeyboard::Initialize();

		glm::vec4 ClearColor{ 0.10f, 0.10f, 0.10f, 1.0f };
		glm::vec4 FragColor{ 1.0f, 0.560f, 1.0f, 1.0f };

		while (Running)
		{
			glfwPollEvents();
			glClearColor(ClearColor.r, ClearColor.g, ClearColor.b, ClearColor.a);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			CTest::ImGui_NewFrame();

			ImGui::Text("%s", LK_TEST_NAME);
			ImGui::Text("Resolution: %dx%d", WindowData.Width, WindowData.Height);

			CKeyboard::Update();

			ImGui::SetNextItemWidth(320.0f);
			ImGui::SliderFloat3("Background", &ClearColor.x, 0.0f, 1.0f, "%.2f");

			const bool bTable = ImGui::BeginTable("##Table", 2);
			ImGui::TableSetupColumn("##Column1");
			ImGui::TableSetupColumn("##Column2");

			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);

			static std::vector<EKey> PressedKeys;
			const std::size_t PressedKeyCount = CKeyboard::GetPressedKeys(PressedKeys);
			const std::string Title = LK_FMT("Pressed Keys: {}", PressedKeyCount);
			if (ImGui::BeginTable(Title.c_str(), 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit))
			{
				ImGui::TableSetupColumn("Key", ImGuiTableColumnFlags_WidthFixed, ImGui::CalcTextSize("Key ------").x);
				ImGui::TableSetupColumn("Repeat Count", ImGuiTableColumnFlags_WidthFixed, 100.0f);
				ImGui::TableSetupColumn("Time Held (ms)", ImGuiTableColumnFlags_WidthFixed, 120.0f);
				ImGui::TableHeadersRow();

				for (const EKey PressedKey : PressedKeys)
				{
					using namespace std::chrono;

					const FKeyData& KeyData = CKeyboard::GetKeyData(PressedKey);
					const auto TimeHeld = CKeyboard::GetKeyHeldTime<milliseconds>(PressedKey);

					ImGui::TableNextRow();

					/* Column 1: Key Name */
					ImGui::TableSetColumnIndex(0);
					ImGui::Text("%s", Enum::ToString(PressedKey));

					/* Column 2: Repeat Count */
					ImGui::TableSetColumnIndex(1);
					ImGui::Text("%d", KeyData.RepeatCount);

					/* Column 3: Time Held in milliseconds */
					ImGui::TableSetColumnIndex(2);
					ImGui::Text("%d", static_cast<int>(TimeHeld.count()));
				}

				ImGui::EndTable();
			}

			ImGui::TableSetColumnIndex(1);

			/****************************
			 * Draw player
			 ****************************/

			ImGui::EndTable();

			CTest::ImGui_EndFrame();
			glfwSwapBuffers(Window.GetGlfwWindow());
			glfwPollEvents();
		}
	}

	void CTest::Destroy()
	{
		glfwTerminate();
	}

}