#pragma once
#include "ComponentPanel.h"

#include <src/Scene/Components/CylinderColliderComponent.h>

class CylinderColliderPanel
{
public:
	static  void Draw(Nuake::Entity& entity, entt::meta_any& componentInstance)
	{
		using namespace Nuake;
        
		Nuake::CylinderColliderComponent* componentPtr = componentInstance.try_cast<Nuake::CylinderColliderComponent>();
		if (componentPtr == nullptr)
		{
			return;
		}
		Nuake::CylinderColliderComponent& component = *componentPtr;
		
		BeginComponentTable(CYLINDER COLLIDER, CylinderColliderComponent)
		{
			{
				ImGui::Text("Radius");
				ImGui::TableNextColumn();
				ImGui::DragFloat("##Radius", &component.Radius, 0.01f, 0.001f);
				component.Radius = std::max(component.Radius, 0.001f);
				ImGui::TableNextColumn();
				ComponentTableReset(component.Radius, 0.5f)
			}
			ImGui::TableNextColumn();
			{
				ImGui::Text("Height");
				ImGui::TableNextColumn();
				ImGui::DragFloat("##Height", &component.Height, 0.01f, 0.0001f);
				component.Height = std::max(component.Height, 0.001f);
				ImGui::TableNextColumn();
				ComponentTableReset(component.Height, 1.0f)
			}
			ImGui::TableNextColumn();
			{
				ImGui::Text("Is Trigger");
				ImGui::TableNextColumn();

				ImGui::Checkbox("##isTrigger", &component.IsTrigger);
				ImGui::TableNextColumn();
				ComponentTableReset(component.IsTrigger, false);
			}
		}
		EndComponentTable()
	}
};