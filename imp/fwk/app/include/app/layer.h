#pragma once
#include <memory>
#include <string>
#include <vector>

namespace imp::fwk
{
    class ILayer
    {
    public:
        explicit ILayer(std::string name) : m_name(std::move(name)) {}
        virtual ~ILayer() = default;

        ILayer(const ILayer&) = delete;
        ILayer& operator=(const ILayer&) = delete;

        virtual void onAttach() {}
        virtual void onDetach() {}

        virtual void onUpdate(float deltaSeconds) { (void)deltaSeconds; }

        const std::string& name() const { return m_name; }
    private:
        std::string m_name;
    };

    class LayerStack
    {
    public:
        LayerStack() = default;
        ~LayerStack();

        LayerStack(const LayerStack&) = delete;
        LayerStack& operator=(const LayerStack&) = delete;

        void pushLayer(std::unique_ptr<ILayer> layer);
        void pushOverlay(std::unique_ptr<ILayer> overlay);

        void popLayer(const std::string& name);
        void popOverlay(const std::string& name);

        void updateAll(float deltaSeconds);

        [[nodiscard]] size_t layerCount() const { return m_layers.size(); }
        [[nodiscard]] size_t overlayCount() const { return m_overlays.size(); }
    private:
        std::vector<std::unique_ptr<ILayer>> m_layers;
        std::vector<std::unique_ptr<ILayer>> m_overlays;
    };
}
