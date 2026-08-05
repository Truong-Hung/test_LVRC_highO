#include "ui/TransferFunction.hpp"

#include "renderer/opengl/utils/Math.hpp"

#include <imgui_internal.h>
#include <algorithm>
#include <cmath>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#ifndef _WIN32
	#include <unistd.h>
#endif

namespace {
    ImVec2 lerp(const ImVec2& a, const ImVec2& b, const ImVec2& t) {
        return ImVec2(
            ::lerp(a.x, b.x, t.x),
            ::lerp(a.y, b.y, t.y)
        );
    }

    ImVec2 invLerp(const ImVec2& a, const ImVec2& b, const ImVec2& v) {
        return ImVec2(
            ::invLerp(a.x, b.x, v.x),
            ::invLerp(a.y, b.y, v.y)
        );
    }

    void clamp01(ImVec2& position) {
        position.x = std::clamp(position.x, 0.f, 1.f);
        position.y = std::clamp(position.y, 0.f, 1.f);
    }

    inline float sqrDistance(const ImVec2& a, const ImVec2& b) {
        ImVec2 ab(
            a.x - b.x,
            a.y - b.y
        );
        return ab.x * ab.x + ab.y * ab.y;
    }

    ImVec2 getPointPosition(const TransferFunction::ControlPoint& values,
                            const ImRect& bb) {
        return lerp(
            bb.Min,
            bb.Max,
            ImVec2(values.x, 1.f - values.y)
        );
    }

    std::tuple<float, float> getPointValues(const ImVec2& position,
                                            const ImRect& bb) {
        auto values = invLerp(bb.Min, bb.Max, position);
        clamp01(values);
        return std::make_tuple(values.x, 1.f - values.y);
    }

    bool isValidPoint(const TransferFunction::ControlPoint point) {
        return point.x >= 0 && point.y >= 0 && point.x <= 1 && point.y <= 1;
    }
}

TransferFunction::TransferFunction(glm::vec4* samples, int samplesCount, std::vector<std::vector<float>>& datas) :
    samples(samples),
    samplesCount(samplesCount)
{
    hoveredPoint_ = {0.00f, 0.00f, -1, {0.f, 0.f, 0.f}};

    // Apply default colormap
    applyPreset(ColormapPreset::Jet);

    // Generate the histogram
    generateHistogram(datas);

    // Generate texture samples
    regenerateSamples();
}

void TransferFunction::applyPreset(ColormapPreset preset)
{
    currentPreset_ = preset;
    middlePoints_.clear();

    // Helper to add a point (re-indexes automatically)
    auto add = [&](float x, float y, float r, float g, float b) {
        middlePoints_.push_back({x, y, (int)middlePoints_.size(), {r, g, b}});
    };

    switch(preset)
    {
    case ColormapPreset::Jet:
        add(0.20f, 0.10f, 0.f,  0.f,  1.f);
        add(0.35f, 0.70f, 0.f,  1.f,  1.f);
        add(0.45f, 1.00f, 0.f,  1.f,  0.f);
        add(0.70f, 1.00f, 1.f,  1.f,  0.f);
        add(1.00f, 1.00f, 1.f,  0.f,  0.f);
        break;

    case ColormapPreset::Viridis:
        // Sampled from matplotlib Viridis
        add(0.00f, 0.05f, 0.267f, 0.005f, 0.329f);
        add(0.25f, 0.40f, 0.229f, 0.322f, 0.545f);
        add(0.50f, 0.75f, 0.128f, 0.566f, 0.551f);
        add(0.75f, 0.90f, 0.370f, 0.788f, 0.383f);
        add(1.00f, 1.00f, 0.993f, 0.906f, 0.144f);
        break;

    case ColormapPreset::Plasma:
        // Sampled from matplotlib Plasma
        add(0.00f, 0.05f, 0.241f, 0.015f, 0.610f);
        add(0.25f, 0.40f, 0.659f, 0.044f, 0.561f);
        add(0.50f, 0.75f, 0.904f, 0.239f, 0.337f);
        add(0.75f, 0.90f, 0.971f, 0.541f, 0.115f);
        add(1.00f, 1.00f, 0.940f, 0.975f, 0.131f);
        break;

    case ColormapPreset::Inferno:
        // Sampled from matplotlib Inferno
        add(0.00f, 0.05f, 0.002f, 0.002f, 0.014f);
        add(0.25f, 0.40f, 0.282f, 0.047f, 0.380f);
        add(0.50f, 0.75f, 0.659f, 0.138f, 0.290f);
        add(0.75f, 0.90f, 0.958f, 0.433f, 0.104f);
        add(1.00f, 1.00f, 0.988f, 1.000f, 0.644f);
        break;

    case ColormapPreset::Grayscale:
        add(0.00f, 0.00f, 0.f, 0.f, 0.f);
        add(1.00f, 1.00f, 1.f, 1.f, 1.f);
        break;

    default:
        break;
    }

    regenerateSamples();
}

void TransferFunction::generateHistogram(std::vector<std::vector<float>>& datas)
{
    dataCount = 0;

    for(uint i = 0; i < 20; i++)
        histogram_[i] = 0;
    
    // For each timestep in the data
    for(uint timestep = 0; timestep < datas.size(); timestep++){
        dataCount += datas[timestep].size();
        // For each datapoint in the timestep
        for(uint data = 0; data < datas[timestep].size(); data++){
            // Classify each datapoint into a bin
            for(uint i = 1; i < 21; i++){
                if(datas[timestep][data] < static_cast<float>(i)/20.f + 1.e-4f){
                    histogram_[i - 1]++;
                    break;
                }
            }
        }
    }
}

int TransferFunction::draw(const char* label, const ImVec2& size)
{
    ImGui::PushID(label);

    // --- Preset combo box ---
    bool presetChanged = false;
    static const char* presetNames[] = {"Custom", "Jet", "Viridis", "Plasma", "Inferno", "Grayscale"};
    int currentPresetIdx = static_cast<int>(currentPreset_);
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    if(ImGui::BeginCombo("##preset", presetNames[currentPresetIdx])) {
        for(int i = 1; i < 6; ++i) { // skip Custom (0) — it is set automatically
            bool selected = (currentPresetIdx == i);
            if(ImGui::Selectable(presetNames[i], selected)) {
                applyPreset(static_cast<ColormapPreset>(i));
                presetChanged = true;
            }
            if(selected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    // Draw the editor
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImGuiWindow* window = ImGui::GetCurrentWindow();

    // Prepare canvas
    const float availableWidth = ImGui::GetContentRegionAvail().x;

    // Set the size of the canvas
    const ImVec2 canvas(
        size.x > 0 ? size.x : availableWidth - ImGui::GetStyle().FramePadding.x,
        size.y > 0 ? size.y : size.x
    );

    // Prepare the bounding box of the canvas
    const ImRect bb(
        ImVec2(
            window->DC.CursorPos.x + ImGui::GetStyle().FramePadding.x, 
            window->DC.CursorPos.y + ImGui::GetStyle().FramePadding.y),
        ImVec2(
            window->DC.CursorPos.x + + ImGui::GetStyle().FramePadding.x + canvas.x,
            window->DC.CursorPos.y + + ImGui::GetStyle().FramePadding.y + canvas.y
        )
    );

    ImGui::ItemSize(bb);

    if(!ImGui::ItemAdd(bb, 0)){
        ImGui::PopID();
        return 0;
    }

    const ImGuiID id = window->GetID(label);
    ImGui::ItemHoverable(bb, id, ImGuiItemFlags_None);

    // Draw background lines
    for(uint i = 0; i <= 10; i++){
        float posX = canvas.x*.1f*static_cast<float>(i);
        float posY = canvas.y*.1f*static_cast<float>(i);

        // Horizontal
        drawList->AddLine(
            ImVec2(bb.Min.x + posX, bb.Min.y),
            ImVec2(bb.Min.x + posX, bb.Max.y),
            ImColor(0.6f, 0.6f, 0.6f, 0.6f),
            1.f
        );

        // Vertical
        drawList->AddLine(
            ImVec2(bb.Min.x, bb.Min.y + posY),
            ImVec2(bb.Max.x, bb.Min.y + posY),
            ImColor(0.6f, 0.6f, 0.6f, 0.6f),
            1.f
        );
    }

    // Draw histogram
    for(uint i = 0; i < 20; i++){
        float height =  static_cast<float>(histogram_[i])/static_cast<float>(dataCount);
        ImVec2 startPos = ImVec2(bb.Min.x + canvas.x*static_cast<float>(i)/20.f, bb.Max.y);
        ImVec2 endPos = ImVec2(bb.Min.x + canvas.x*static_cast<float>(i + 1)/20.f, bb.Max.y - canvas.y*height);

        drawList->AddRectFilled(startPos, endPos, ImColor(.7f, .7f, .7f, .7f));   
    }

    // Draw samples
    bool changed = false;

    // Handle user interactions
    handleHoveredPoint(bb);
    changed |= handlerInputs(bb);

    // Draw all the curves
    drawCurve(drawList, bb);

    ImGui::PopID();
    ImGui::Dummy({ 0, 20 });

    if(changed) {
        // User edited manually -> switch to Custom
        currentPreset_ = ColormapPreset::Custom;
        regenerateSamples();
    }

    return changed || presetChanged;
}

void TransferFunction::regenerateSamples()
{
    for(uint i = 0; i < samplesCount; i++)
        samples[i] = getSample(static_cast<float>(i)/static_cast<float>(samplesCount));
}

glm::vec4 TransferFunction::getSample(float xValue) const
{
    glm::vec4 sample(0.f, 0.f, 0.f, 0.f);

    if(middlePoints_.empty())
        return sample;

    // Find the previous and next control point
    auto followingCtrlPIt = std::find_if(middlePoints_.begin(),
                                         middlePoints_.end(),
                                         [xValue](const auto& values){return values.x > xValue;});           
    int pos = static_cast<int>(std::distance(middlePoints_.begin(), followingCtrlPIt));

    ControlPoint previousValues = (pos == 0) ? 
        ControlPoint({0.f, 0.f, -1, middlePoints_[pos].rgb}) : middlePoints_[pos - 1];

    ControlPoint nextValues = followingCtrlPIt == middlePoints_.end() ? 
        ControlPoint{1.f, 0.f, -1, middlePoints_.back().rgb} : *followingCtrlPIt;

    // Compute the sample value by interpolating between the previous and next point
    const float t = invLerp(previousValues.x, nextValues.x, xValue);

    sample = glm::vec4(
        lerp(previousValues.rgb.r, nextValues.rgb.r, t),
        lerp(previousValues.rgb.g, nextValues.rgb.g, t),
        lerp(previousValues.rgb.b, nextValues.rgb.b, t),
        lerp(previousValues.y, nextValues.y, t)
    );

    return sample;
}

void TransferFunction::drawCurve(ImDrawList* drawList, const ImRect& bb)
{
    auto& controlPoints = middlePoints_;

    // Draw the curve
    ImColor lineColor = ImColor(0.5f, 0.5f, 0.5f, 1.f);

    if(controlPoints.empty()){
        drawList->AddLine(getPointPosition({0, 0, -1, {0.f, 0.f, 0.f}}, bb),
                          getPointPosition({1, 1, -1, {0.f, 0.f, 0.f}}, bb),
                          lineColor,
                          2.f);
    }else{
        for(int i = -1; i < (int)controlPoints.size(); i++){
            ControlPoint first, second;
            if (i == -1) 
                first = {0.f, 0.f, -1, controlPoints[0].rgb};
            else 
                first = controlPoints[i];
            if (i == (int)controlPoints.size() - 1)
                second = {1.f, 0.f, -1, controlPoints[i].rgb};
            else
                second = controlPoints[i + 1];
            
            lineColor = ImColor(
                lerp(first.rgb.r, second.rgb.r, 0.5f),
                lerp(first.rgb.g, second.rgb.g, 0.5f),
                lerp(first.rgb.b, second.rgb.b, 0.5f),
                1.f
            );
            
            drawList->AddLine(
                getPointPosition(first, bb),
                getPointPosition(second, bb),
                lineColor,
                1.f
            );
        }
    }

    // Draw the control points
    for(uint i = 0; i < controlPoints.size(); i++){
        ControlPoint& ctrlPoint = controlPoints[i];
        const auto position = getPointPosition(ctrlPoint, bb);
        ImColor color = ImColor(ctrlPoint.rgb.r, ctrlPoint.rgb.g, ctrlPoint.rgb.b, 1.f);
    
        drawList->AddCircleFilled(position, pointsRadius_, color);

        if (isValidPoint(hoveredPoint_) && (size_t)hoveredPoint_.index == i){
            drawList->AddCircle(
                position, hoverRadius_,
                ImColor(1.f, 1.f, 1.f, 1.f)
            );

            ImGui::SetTooltip(
                "x = %1.5f | y = %1.5f",
                ctrlPoint.x,
                ctrlPoint.y
            );
        }
    }
}

void TransferFunction::handleHoveredPoint(const ImRect& bb)
{
    const ImVec2 mousePos = ImGui::GetIO().MousePos;

    ControlPoint nearestPoint;
    auto nearestPointSqrDist = std::numeric_limits<float>::infinity();

    // Find the nearest point to the mouse
    for (size_t j = 0; j < middlePoints_.size(); ++j) {
        const auto guiPos = getPointPosition(middlePoints_[j], bb);
        const auto sqrDist = sqrDistance(guiPos, mousePos);

        if (sqrDist < nearestPointSqrDist) {
            nearestPointSqrDist = sqrDist;
            nearestPoint = middlePoints_[j];
            nearestPoint.index = static_cast<int>(j);
        }
    }

    // If the mouse is over a point, set the hovered point
    if (nearestPointSqrDist < hoverRadius2_) {
        hoveredPoint_ = nearestPoint;
    } else {
    // Otherwise, set the hovered point to invalid
        hoveredPoint_ = {-1.f, -1.f, -1, {0.f, 0.f, 0.f}};
    }
}

bool TransferFunction::handlerInputs(const ImRect& bb)
{
    const auto& mousePosition = ImGui::GetIO().MousePos;

    auto handleDragging = [this, mousePosition, bb]() -> bool {
        if (!ImGui::IsPopupOpen("edit_point_popup")
            && ImGui::IsMouseDragging(0, 0.1f)) {
            
            // If the current dragging point is not valid, set the dragging point to the hovered point
            if (!isValidPoint(draggedPoint_)) {
                if(ImGui::IsMouseDragging(ImGuiMouseButton_Left)){
                    // No points are selected : update the iso-value
                    if(bb.Contains(mousePosition)){
                        iso_value = (mousePosition.x - bb.Min.x)/(bb.Max.x - bb.Min.x)*(max - min) + min;

                        if (ImGui::BeginTooltip()){
                            ImGui::Text(("Iso-value : " + std::to_string(iso_value)).c_str());
                            ImGui::EndTooltip();
                        }

                        return true;
                    }

                    return false;
                }else{
                    if(isValidPoint(hoveredPoint_)){
                        draggedPoint_ = hoveredPoint_;
                    }else{
                        return false;
                    }
                }
            }

            // Get the position of the dragged point in the vector
            auto index = draggedPoint_.index;

            auto point = getPointValues(mousePosition, bb);
            // Erase the point from the vector
            middlePoints_.erase(
                middlePoints_.begin()
                + index
            );
            // Find the next point to the mouse position
            auto pos = std::find_if(
                middlePoints_.begin(),
                middlePoints_.end(),
                [point](const auto& p) {
                    return p.x >= std::get<0>(point);
                }
            );
            // Set the new point information and insert it in the vector
            auto newIndex = std::distance(
                middlePoints_.begin(),
                pos
            );
            ControlPoint insertPoint = {
                std::get<0>(point),
                std::get<1>(point),
                (int)newIndex,
                draggedPoint_.rgb
            };
            middlePoints_.insert(pos, insertPoint);
            draggedPoint_ = insertPoint;

            return true;
        } else {
            draggedPoint_ = {-1.f, -1.f, -1, {0.f, 0.f, 0.f}};
            return false;
        }
    };

    auto handleInsertPoint = [this, mousePosition, bb]() -> bool {
        if (!ImGui::IsPopupOpen("edit_point_popup")
         && ImGui::IsMouseDoubleClicked(0)
         && bb.Contains(mousePosition)) {
            auto point = getPointValues(mousePosition, bb);
            glm::vec3 rgb;
            // Find the next point to the mouse position
            auto pos = std::find_if(
                middlePoints_.begin(),
                middlePoints_.end(),
                [point, &rgb](const auto& p) {
                    if (p.x > std::get<0>(point)) {
                        rgb = p.rgb;
                        return true;
                    } 
                    return false;               
                }
            );
            auto newIndex = std::distance(
                middlePoints_.begin(),
                pos
            );
            // Update position for other points
            for (size_t i = (size_t)newIndex; i < middlePoints_.size(); ++i) {
                middlePoints_[i].index++;
            }
            // Create the new point and insert it in the vector
            ControlPoint insertPoint = {
                std::get<0>(point),
                std::get<1>(point),
                (int)newIndex,
                rgb
            };
            middlePoints_.insert(pos, insertPoint);
            return true;
        }
        return false;
    };

    auto handleEditPoint = [this]() -> bool {
        // If hovered point is valid, open the edit point popup
        if(ImGui::IsMouseClicked(1) && isValidPoint(hoveredPoint_)){
            editedPoint_ = hoveredPoint_;
            ImGui::OpenPopup("edit_point_popup");
        }else{
            if(!ImGui::IsPopupOpen("edit_point_popup"))
                editedPoint_ = {-1.f, -1.f, -1, {0.f, 0.f, 0.f}};
        }

        bool edited = false;
        std::tuple<float, float> point = std::make_tuple(editedPoint_.x, editedPoint_.y);

        if (ImGui::BeginPopup("edit_point_popup")) {
            edited |= ImGui::ColorEdit3("RGB##edit_point_popup", glm::value_ptr(editedPoint_.rgb));

            if(ImGui::Button("Delete##edit_point_popup")){
                middlePoints_.erase(middlePoints_.begin() + editedPoint_.index);
                edited = true;
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
        
        if(edited){
            auto pos = std::find_if(
                middlePoints_.begin(),
                middlePoints_.end(),
                [point](const auto& p) {
                    return p.x >= std::get<0>(point);
                }
            );
            
            *pos = editedPoint_;
        }
        return edited;
    };

    return handleDragging() || handleInsertPoint() || handleEditPoint();
}