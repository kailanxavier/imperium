#include <fwk/layer.h>
#include <algorithm>

namespace imp::fwk
{
    namespace
    {
        void popByName(std::vector<std::unique_ptr<ILayer>>& layers, const std::string& name, app::AppContext* ctx)
        {
            auto it = std::find_if(layers.begin(), layers.end(),
                [&](const std::unique_ptr<ILayer>& l) { return l->name() == name; });

            if (it != layers.end())
            {
                if (ctx)
                    (*it)->onDetach(*ctx);
                layers.erase(it);
            }
        }
    }

    LayerStack::~LayerStack()
    {
        clear();
    }

    void LayerStack::attachNow(std::unique_ptr<ILayer> layer, bool overlay)
    {
        layer->onAttach(*m_ctx);
        ( overlay ? m_overlays : m_layers ).push_back(std::move(layer));
    }

    void LayerStack::pushLayer(std::unique_ptr<ILayer> layer)
    {
        if (m_ctx)
            attachNow(std::move(layer), false);
        else
            m_pending.push_back({ std::move(layer), false });
    }

    void LayerStack::pushOverlay(std::unique_ptr<ILayer> overlay)
    {
        if (m_ctx)
            attachNow(std::move(overlay), true);
        else
            m_pending.push_back({ std::move(overlay), true });
    }

    void LayerStack::attachPending(app::AppContext& ctx)
    {
        m_ctx = &ctx;
        std::vector<Pending> queued = std::move(m_pending);
        m_pending.clear();
        
        for (auto& p : queued)
            attachNow(std::move(p.layer), p.overlay);
    }

    void LayerStack::popLayer(const std::string &name) 
    {
        popByName(m_layers, name, m_ctx);
        if (!m_ctx)
            std::erase_if(m_pending, [&](const Pending& p) 
                { 
                    return !p.overlay && p.layer->name() == name; 
                });
    }
    void LayerStack::popOverlay(const std::string &name) 
    { 
        popByName(m_overlays, name, m_ctx);
        if (!m_ctx)
            std::erase_if(m_pending, [&](const Pending& p)
                {
                    return p.overlay && p.layer->name() == name;
                });
    }

    void LayerStack::updateAll(float deltaSeconds)
    {
        if (!m_ctx)
            return;

        for (auto& l : m_layers) l->onUpdate(*m_ctx, deltaSeconds);
        for (auto& l : m_overlays) l->onUpdate(*m_ctx, deltaSeconds);
    }

    void LayerStack::renderAll(gfx::ICommandList& cmd)
    {
        if (!m_ctx)
            return;
        for (auto& l : m_layers) l->onRender(*m_ctx, cmd);
        for (auto& l : m_overlays) l->onRender(*m_ctx, cmd);
    }

    void LayerStack::clear()
    {
        if (m_ctx)
        {
            for (auto it = m_overlays.rbegin(); it != m_overlays.rend(); ++it) ( *it )->onDetach(*m_ctx);
            for (auto it = m_layers.rbegin(); it != m_layers.rend(); ++it) ( *it )->onDetach(*m_ctx);
        }

        m_overlays.clear();
        m_layers.clear();
        m_pending.clear();
        m_ctx = nullptr;
    }
}
