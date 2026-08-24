#ifndef LVRC_GUI_HPP
#define LVRC_GUI_HPP

#include <imgui.h>
#include <string>
#include <tuple>
#include <vector>
#include <bitset>
#include <array>
#include <glm/vec4.hpp>
#include <glm/vec3.hpp>
#include <imgui_internal.h>

class TransferFunction
{
public:
    // Control point of the transfer function curve
    struct ControlPoint{
        float x;
        float y;
        int index;
        glm::vec3 rgb;
    };

    enum class ColormapPreset {
        Custom = 0,
        Jet,
        Viridis,
        Plasma,
        Inferno,
        Grayscale
    };

public:
    // Control point UI parameters
    float pointsRadius_ = 5.f;
    float hoverRadius_ = 2.f*pointsRadius_;
    float hoverRadius2_ = hoverRadius_*hoverRadius_;

public:
    TransferFunction(glm::vec4* samples, int samplesCount, std::vector<std::vector<float>> &data); 

    int draw(const char* label, const ImVec2& size = ImVec2(0, 200));

    [[nodiscard]] float get_iso_value() const { return iso_value; }
    [[nodiscard]] float get_iso_normalized() const { return (iso_value - min) / (max - min); }
    [[nodiscard]] float get_min() const { return min; }
    [[nodiscard]] float get_max() const { return max; }
    void set_min(float v) { min = v; }
    void set_max(float v) { max = v; }

    void generateHistogram(std::vector<std::vector<float>>& datas);
    void regenerateSamples();
    void applyPreset(ColormapPreset preset);

    [[nodiscard]] glm::vec4 getSample(float xValue) const;

private:
    void drawCurve(ImDrawList* drawList, const ImRect& bb);

    void handleHoveredPoint(const ImRect& bb);
    bool handlerInputs(const ImRect& bb);


private:
    // Values
    float iso_value = .5f;
    float min = 0.f;
    float max = 1.f;
    
    // Preset
    ColormapPreset currentPreset_ = ColormapPreset::Jet;
    
    // Control points
    ControlPoint firstPoint_;
    ControlPoint secondPoint_;
    std::vector<ControlPoint> middlePoints_;

    // Histogram
    uint dataCount;
    uint histogram_[20];

    // Interaction points
    ControlPoint hoveredPoint_;
    ControlPoint draggedPoint_;
    ControlPoint editedPoint_;

    // Samples
    glm::vec4* samples;
    uint samplesCount;
};

#endif //LVRC_GUI_HPP
