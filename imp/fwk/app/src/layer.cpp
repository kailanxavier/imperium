#include "fwk/layer.h"
#include <algorithm>

namespace imp::fwk
{
    namespace
    {
        void popByName(std::vector<std::unique_ptr<ILayer>>& layers, const std::string& name)
        {
            auto it = std::find_if(layers.begin(), layers.end(),
                [&](const std::unique_ptr<ILayer>& l) { return l->name() == name; });

            if (it != layers.end())
            {
                (*it)->onDetach();
                layers.erase(it);
            }
        }
    }

    LayerStack::~LayerStack()
    {
        for (auto& l : m_overlays) l->onDetach();
        for (auto& l : m_layers) l->onDetach();
    }

    void LayerStack::pushLayer(std::unique_ptr<ILayer> layer)
    {
        layer->onAttach();
        m_layers.push_back(std::move(layer));
    }

    void LayerStack::pushOverlay(std::unique_ptr<ILayer> overlay)
    {
        overlay->onAttach();
        m_overlays.push_back(std::move(overlay));
    }

    void LayerStack::popLayer(const std::string &name) { popByName(m_layers, name); }
    void LayerStack::popOverlay(const std::string &name) { popByName(m_overlays, name); }

    void LayerStack::updateAll(float deltaSeconds)
    {
        for (auto& l : m_layers) l->onUpdate(deltaSeconds);
        for (auto& l : m_overlays) l->onUpdate(deltaSeconds);
    }

}
