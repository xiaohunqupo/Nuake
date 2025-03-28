#include "Engine.h"

#include "src/Scene/Scene.h"
#include "src/Resource/Project.h"
#include "src/Physics/PhysicsManager.h"

#include "src/AI/NavManager.h"
#include "src/Audio/AudioManager.h"
#include "src/FileSystem/FileSystem.h"
#include "src/Core/Input.h"
#include "src/Rendering/Renderer.h"
#include "src/Rendering/Renderer2D.h"
#include "src/Scripting/ScriptingEngineNet.h"
#include "src/Threading/JobSystem.h"
#include "src/Core/RegisterCoreTypes.h"
#include "src/Modules/Modules.h"
#include "src/Subsystems/EngineSubsystemScriptable.h"

#include <GLFW/glfw3.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>
#include <Tracy.hpp>



namespace Nuake
{
	Ref<Project> Engine::currentProject;
	Ref<Window> Engine::currentWindow;
	std::string Engine::queuedScene = "";

	GameState Engine::gameState = GameState::Stopped;

	float Engine::lastFrameTime = 0.0f;
	float Engine::fixedUpdateRate = 1.0f / 90.0f;
	float Engine::fixedUpdateDifference = 0.f;
	float Engine::time = 0.f;
	Timestep Engine::timeStep = 0.f;
	float Engine::timeScale = 1.0f;

	void Engine::Init()
	{
		//Window::Get()->OnWindowSetScene().AddStatic(&Engine::OnWindowSetScene);
		
		ScriptingEngineNet::Get().OnGameAssemblyLoaded().AddStatic(&Engine::OnScriptingEngineGameAssemblyLoaded);
		
		AudioManager::Get().Initialize();
		PhysicsManager::Get().Init();
		NavManager::Get().Initialize();

		// Creates the window
		currentWindow = Window::Get();

		Input::Init();
		Renderer2D::Init();
		Logger::Log("Engine initialized");

		RegisterCoreTypes::RegisterCoreComponents();

		Modules::StartupModules();

		InitializeCoreSubsystems();
	}

	void Engine::Tick()
	{
		ZoneScoped;

		JobSystem::Get().Update();

		time = static_cast<float>(glfwGetTime());
		timeStep = time - lastFrameTime;
		lastFrameTime = time;

		if (Engine::IsPlayMode())
		{
			if (!queuedScene.empty())
			{
				Ref<Scene> nextScene = Scene::New();
				if (FileSystem::FileExists(queuedScene))
				{
					const std::string& fileContent = FileSystem::ReadFile(queuedScene);
					nextScene->Path = queuedScene;
					nextScene->Deserialize(json::parse(fileContent));

					// Uninit current scene
					GetCurrentScene()->OnExit();

					// Set new scene
					SetCurrentScene(nextScene);

					// Init new scene
					PhysicsManager::Get().ReInit();
					GetCurrentScene()->OnInit();

					queuedScene = "";
				}
				
			}
		}
		
		float scaledTimeStep = timeStep * timeScale;

		// Tick all subsystems
		if (Engine::IsPlayMode())
		{
			for (auto subsystem : subsystems)
			{
				if (subsystem == nullptr)
					continue;

				if (subsystem->CanEverTick())
				{
					subsystem->Tick(scaledTimeStep);
				}
			}
		}

		// Dont update if no scene is loaded.
		if (currentWindow->GetScene())
		{
			currentWindow->Update(scaledTimeStep);

			// Play mode update all the entities, Editor does not.
			if (!Engine::IsPlayMode())
			{
				GetCurrentScene()->EditorUpdate(scaledTimeStep);
			}

			fixedUpdateDifference += timeStep;

			// Fixed update
			while (fixedUpdateDifference >= fixedUpdateRate)
			{
				currentWindow->FixedUpdate(fixedUpdateRate * timeScale);

				fixedUpdateDifference -= fixedUpdateRate;
			}

			Input::Update();
			AudioManager::Get().AudioUpdate();
		}
	}

	void Engine::EnterPlayMode()
	{
		lastFrameTime = (float)glfwGetTime(); // Reset timestep timer.

		// Dont trigger init if already in player mode.
		if (GetGameState() == GameState::Playing || GetGameState() == GameState::Loading)
		{
			Logger::Log("Cannot enter play mode if is already in play mode or is loading.", "engine", WARNING);
			return;
		}
		
		SetGameState(GameState::Loading);

		PhysicsManager::Get().ReInit();

		if (GetCurrentScene()->OnInit())
		{
			SetGameState(GameState::Playing);
		}
		else
		{
			Logger::Log("Cannot enter play mode. Scene OnInit failed", "engine", CRITICAL);
			GetCurrentScene()->OnExit();
		}
	}

	void Engine::ExitPlayMode()
	{
		// Dont trigger exit if already not in play mode.
		if (gameState != GameState::Stopped)
		{
			GetCurrentScene()->OnExit();
			Input::ShowMouse();
			gameState = GameState::Stopped;
		}
	}

	void Engine::Draw()
	{
		ZoneScoped;

		RenderCommand::Clear();

		// Start imgui frame
		{
			ZoneScopedN("ImGui New Frame");

			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();
		}

		// Draw scene
		Window::Get()->Draw();
	}

	void Engine::EndDraw()
	{
		ZoneScoped;
		Window::Get()->EndDraw();
	}

	void Engine::Close()
	{
		glfwTerminate();
	}

	Ref<Scene> Engine::GetCurrentScene()
	{
		if (currentWindow)
		{
			return currentWindow->GetScene();
		}

		return nullptr;
	}

	bool Engine::SetCurrentScene(Ref<Scene> scene)
	{
		return currentWindow->SetScene(scene);
	}

	bool Engine::QueueSceneSwitch(const std::string& scenePath)
	{
		if (!IsPlayMode())
		{
			return false;
		}

		queuedScene = scenePath;

		return true;
	}

	Ref<Project> Engine::GetProject()
	{
		return currentProject;
	}

	Ref<EngineSubsystemScriptable> Engine::GetScriptedSubsystem(const std::string& subsystemName)
	{
		if (scriptedSubsystemMap.contains(subsystemName))
		{
			return scriptedSubsystemMap[subsystemName];
		}
		return nullptr;
	}

	Ref<EngineSubsystemScriptable> Engine::GetScriptedSubsystem(const int subsystemId)
	{
		if (subsystemId >= subsystems.size())
		{
			return nullptr;
		}
		return std::reinterpret_pointer_cast<EngineSubsystemScriptable>(subsystems[subsystemId]);
	}

	void Engine::OnWindowSetScene(Ref<Scene> oldScene, Ref<Scene> newScene)
	{
		// Inform the subsystems that we are going to destroy/swap out the old scene
		for (auto subsystem : subsystems)
		{
			if (subsystem == nullptr)
				continue;

			subsystem->OnScenePreDestroy(oldScene);
		}

		// Hook into when the internal pieces of the scene are just about to be ready and when the scene is finally
		// ready to present (ie, all initialized and loaded).
		if (newScene != nullptr)
		{
			newScene->OnPreInitialize().AddStatic(&Engine::OnScenePreInitialize, newScene);
			newScene->OnPostInitialize().AddStatic(&Engine::OnScenePostInitialize, newScene);
		}
	}

	void Engine::InitializeCoreSubsystems()
	{
	}

	void Engine::OnScriptingEngineGameAssemblyLoaded()
	{
		if (!Engine::IsPlayMode() && Engine::GetGameState() != GameState::Loading)
		{
			return;
		}
		
		subsystems.clear();
		scriptedSubsystemMap.clear();

		const Coral::ManagedAssembly& gameAssembly = ScriptingEngineNet::Get().GetGameAssembly();

		const auto scriptTypeEngineSubsystem = gameAssembly.GetType("Nuake.Net.EngineSubsystem");
		
		const auto& types = gameAssembly.GetTypes();
		for (const auto& type : types)
		{
			// Initialize all subsystems
			if (type->IsSubclassOf(scriptTypeEngineSubsystem))
			{
				const std::string typeName = std::string(type->GetFullName());
				Logger::Log("Creating Scripted Subsystem " + typeName);

				Coral::ManagedObject scriptedSubsystem = type->CreateInstance();
				scriptedSubsystem.SetPropertyValue("EngineSubsystemID", subsystems.size());
				Ref<EngineSubsystemScriptable> subsystemScript = CreateRef<EngineSubsystemScriptable>(scriptedSubsystem);
				subsystems.push_back(subsystemScript);

				scriptedSubsystemMap[typeName] = subsystemScript;

				subsystemScript->Initialize();
			}
		}
	}

	void Engine::OnScenePreInitialize(Ref<Scene> scene)
	{
		for (auto subsystem : subsystems)
		{
			if (subsystem == nullptr)
				continue;

			subsystem->OnScenePreInitialize(scene);
		}
	}

	void Engine::OnScenePostInitialize(Ref<Scene> scene)
	{
		for (auto subsystem : subsystems)
		{
			if (subsystem == nullptr)
				continue;

			subsystem->OnScenePostInitialize(scene);
		}
	}

	bool Engine::LoadProject(Ref<Project> project)
	{
		currentProject = project;

		if (!Engine::SetCurrentScene(currentProject->DefaultScene))
		{
			return false;
		}

		FileSystem::SetRootDirectory(FileSystem::GetParentPath(project->FullPath));
		ScriptingEngineNet::Get().Initialize();
		ScriptingEngineNet::Get().LoadProjectAssembly(project);

		return true;
	}

	Ref<Window> Engine::GetCurrentWindow()
	{
		return currentWindow;
	}
}
