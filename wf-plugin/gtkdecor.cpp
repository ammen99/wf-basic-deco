#include <wayfire/core.hpp>
#include <wayfire/geometry.hpp>
#include <wayfire/nonstd/wlroots-full.hpp>
#include <wayfire/object.hpp>
#include <wayfire/plugin.hpp>
#include <wayfire/render-manager.hpp>
#include <wayfire/decorator.hpp>
#include <wayfire/debug.hpp>
#include <wayfire/opengl.hpp>
#include <wayfire/scene-render.hpp>
#include <wayfire/scene.hpp>
#include <wayfire/toplevel.hpp>
#include <wayfire/scene-operations.hpp>

#include <wayfire/txn/transaction-object.hpp>
#include <wayfire/txn/transaction-manager.hpp>

#include <type_traits>
#include <wayfire/util.hpp>
#include <wayfire/view.hpp>
#include <wayfire/singleton-plugin.hpp>

#include <wayfire/signal-definitions.hpp>
#include "wf-decorator-protocol.h"

#include <wayfire/unstable/wlr-surface-node.hpp>
#include <wayfire/unstable/wlr-view-events.hpp>
#include <wayfire/unstable/translation-node.hpp>

#define PRIV_COMMIT "_gtk3-deco-priv-commit"

static constexpr int margin_left = 31;
static constexpr int margin_top = 60;
static constexpr int margin_right = 31;
static constexpr int margin_bottom = 35;

using decoration_node_t = std::shared_ptr<wf::scene::wlr_surface_node_t>;

std::ostream& operator << (std::ostream& out, const wf::dimensions_t& dims)
{
    out << dims.width << "x" << dims.height;
    return out;
}

class gtk3_frame_t : public wf::decorator_frame_t_t
{
  public:
    wf::decoration_margins_t get_margins() final
    {
        return {
            .left = 5,
            .right = 5,
            .bottom = 5,
            .top = 38,
        };
    }
};

/**
 * A node which cuts out a part of its children (visually).
 */
class gtk3_mask_node_t : public wf::scene::floating_inner_node_t
{
  public:
    // The 'allowed' portion of the children
    wf::region_t allowed;

    gtk3_mask_node_t() : floating_inner_node_t(false)
    {}

    std::optional<wf::scene::input_node_t> find_node_at(const wf::pointf_t& at) override
    {
        if (allowed.contains_pointf(at))
        {
            return wf::scene::floating_inner_node_t::find_node_at(at);
        }

        return {};
    }

    void gen_render_instances(std::vector<wf::scene::render_instance_uptr>& instances,
        wf::scene::damage_callback push_damage, wf::output_t *output) override
    {
        instances.push_back(std::make_unique<gtk3_mask_render_instance_t>(this, push_damage, output));
    }

    class gtk3_mask_render_instance_t : public wf::scene::render_instance_t
    {
        std::vector<wf::scene::render_instance_uptr> children;
        wf::scene::damage_callback damage_cb;
        gtk3_mask_node_t *self;

        wf::signal::connection_t<wf::scene::node_damage_signal> on_self_damage =
            [=] (wf::scene::node_damage_signal *ev)
        {
            damage_cb(ev->region);
        };

      public:
        gtk3_mask_render_instance_t(gtk3_mask_node_t *self, wf::scene::damage_callback damage_cb,
            wf::output_t *output)
        {
            this->self = self;
            this->damage_cb = damage_cb;
            for (auto& ch : self->get_children())
            {
                ch->gen_render_instances(children, damage_cb, output);
            }
        }

        void schedule_instructions(std::vector<wf::scene::render_instruction_t>& instructions,
            const wf::render_target_t& target, wf::region_t& damage) override
        {
            auto child_damage = damage & self->allowed;
            for (auto& ch : children)
            {
                ch->schedule_instructions(instructions, target, child_damage);
            }
        }

        void render(const wf::render_target_t&, const wf::region_t&) override
        {
            wf::dassert(false, "Rendering a gtk3_mask_node?");
        }

        void compute_visibility(wf::output_t *output, wf::region_t& visible) override
        {
            for (auto& ch : children)
            {
                ch->compute_visibility(output, visible);
            }
        }
    };
};

class gtk3_decoration_object_t : public wf::txn::transaction_object_t
{
    enum class gtk3_decoration_tx_state
    {
        // No transactions in flight
        STABLE,
        // Transaction has just started
        START,
        // The decoration client has ACKed our initial size request. However, the decorated toplevel's client
        // has not ACKed the request yet, so we do not know the actual 'final' size of the client.
        TENTATIVE,
        // Decorated toplevel has set its final size, waiting for the decoration to respond.
        WAITING_FINAL,
    };

  public:
    std::string stringify() const
    {
        std::ostringstream out;
        out << "gtk3deco(" << this << ")";
        return out.str();
    }

    void set_pending_size(wf::dimensions_t desired)
    {
        if (!toplevel)
        {
            return;
        }

        this->pending = desired;
    }

    void set_final_size(wf::dimensions_t final)
    {
        if (!toplevel)
        {
            return;
        }

        LOGI("Final size is ", final, " state is ", (int)deco_state);

        if (this->committed == final)
        {
            switch (this->deco_state)
            {
                case gtk3_decoration_tx_state::STABLE:
                  return;

                case gtk3_decoration_tx_state::START:
                  this->deco_state = gtk3_decoration_tx_state::WAITING_FINAL;
                  break;

                case gtk3_decoration_tx_state::WAITING_FINAL:
                  break;

                case gtk3_decoration_tx_state::TENTATIVE:
                  // fallthrough
                  this->deco_state = gtk3_decoration_tx_state::STABLE;
                  wf::txn::emit_object_ready(this);
                  break;
            }

            return;
        }

        this->committed = final;
        wlr_xdg_toplevel_set_size(toplevel, final.width, final.height);
        this->deco_state = gtk3_decoration_tx_state::WAITING_FINAL;
    }

    void size_updated()
    {
        wlr_box box;
        wlr_xdg_surface_get_geometry(toplevel->base, &box);

        if (wf::dimensions(box) != committed)
        {
            return;
        }
        LOGI("Size is ", wf::dimensions(box), " state is ", (int)deco_state);

        switch (this->deco_state)
        {
            case gtk3_decoration_tx_state::STABLE:
              // Client simply committed, nothing has changed
              return;

            case gtk3_decoration_tx_state::TENTATIVE:
              // Client commits twice?
              return;

            case gtk3_decoration_tx_state::START:
              deco_state = gtk3_decoration_tx_state::TENTATIVE;
              break;

            case gtk3_decoration_tx_state::WAITING_FINAL:
              deco_state = gtk3_decoration_tx_state::STABLE;
              wf::txn::emit_object_ready(this);
              break;
        }
    }

    void commit()
    {
        if (!toplevel)
        {
            wf::txn::emit_object_ready(this);
            return;
        }

        deco_state = gtk3_decoration_tx_state::START;

        LOGI("Committing with ", pending);

        wlr_box box;
        wlr_xdg_surface_get_geometry(toplevel->base, &box);
        if (wf::dimensions(box) != pending)
        {
            wlr_xdg_toplevel_set_size(toplevel, pending.width, pending.height);
        }

        committed = pending;
        size_updated();
    }

    void apply()
    {
        if (toplevel)
        {
            pending_state.merge_state(toplevel->base->surface);
        }

        deco_node->apply_state(std::move(pending_state));
        recompute_mask();
    }

  public:
    gtk3_decoration_object_t(wlr_xdg_toplevel *toplevel, decoration_node_t deco_node,
        std::weak_ptr<gtk3_mask_node_t> mask)
    {
        this->toplevel = toplevel;
        this->deco_node = deco_node;
        this->mask_node = mask;

        on_commit.set_callback([=] (void*)
        {
            pending_state.merge_state(toplevel->base->surface);
            if (deco_state == gtk3_decoration_tx_state::STABLE)
            {
                deco_node->apply_state(std::move(pending_state));
                recompute_mask();
            }

            size_updated();
        });

        on_destroy.set_callback([=] (void*)
        {
            this->toplevel = nullptr;
            switch (deco_state)
            {
                case gtk3_decoration_tx_state::STABLE:
                  break;
                case gtk3_decoration_tx_state::START:
                  // fallthrough
                case gtk3_decoration_tx_state::TENTATIVE:
                  // fallthrough
                case gtk3_decoration_tx_state::WAITING_FINAL:
                  deco_state = gtk3_decoration_tx_state::STABLE;
                  wf::txn::emit_object_ready(this);
                  break;
            }
        });

        on_commit.connect(&toplevel->base->surface->events.commit);
        on_destroy.connect(&toplevel->base->events.destroy);
    }

  private:
    wf::dimensions_t pending = {0, 0};
    wf::dimensions_t committed = {0, 0};

    void recompute_mask()
    {
        auto masked = mask_node.lock();
        wf::dassert(masked != nullptr, "Masked node does not exist anymore??");

        auto bbox = deco_node->get_bounding_box();

        //masked->allowed = wf::geometry_t{-100000, -10000, 10000000, 1000000};
        masked->allowed = bbox;
        wf::region_t cut_out = wf::geometry_t {
            .x = bbox.x + margin_left,
            .y = bbox.y + margin_top,
            .width = bbox.width - margin_left - margin_right,
            .height = bbox.height - margin_top - margin_bottom,
        };
        masked->allowed ^= cut_out;
    }

  private:
    wf::scene::surface_state_t pending_state;
    std::weak_ptr<gtk3_mask_node_t> mask_node;

    wlr_xdg_toplevel *toplevel;
    decoration_node_t deco_node;

    wf::wl_listener_wrapper on_commit, on_destroy;
    gtk3_decoration_tx_state deco_state = gtk3_decoration_tx_state::STABLE;
};

static bool begins_with(const std::string& a, const std::string& b)
{
    return a.substr(0, b.size()) == b;
}

static const std::string gtk_decorator_prefix = "__wf_decorator:";

void do_update_borders(wl_client*, struct wl_resource*, uint32_t, uint32_t, uint32_t, uint32_t)
{
    /* TODO: implement update_borders */
}

const struct wf_decorator_manager_interface decorator_implementation =
{
    .update_borders = do_update_borders
};

wl_resource *decorator_resource = NULL;
void unbind_decorator(wl_resource*)
{
    decorator_resource = NULL;
}

void bind_decorator(wl_client *client, void *, uint32_t, uint32_t id)
{
    LOGI("Binding wf-decorator");
    auto resource = wl_resource_create(client, &wf_decorator_manager_interface, 1, id);

    /* TODO: track active clients */
    wl_resource_set_implementation(resource, &decorator_implementation, NULL, NULL);
    decorator_resource = resource;
}

class gtk3_toplevel_custom_data : public wf::custom_data_t
{
  public:
    std::shared_ptr<gtk3_decoration_object_t> decoration;
};

class gtk3_decoration_plugin : public wf::plugin_interface_t
{
  public:
    wl_global *decorator_global;

    wf::signal::connection_t<wf::new_xdg_surface_signal> on_new_xdg_surface =
        [=] (wf::new_xdg_surface_signal *ev)
    {
        if (ev->surface->role != WLR_XDG_SURFACE_ROLE_TOPLEVEL)
        {
            return;
        }

        auto toplevel = ev->surface->toplevel;
        if (!begins_with(nonull(toplevel->title), gtk_decorator_prefix))
        {
            return;
        }

        LOGI("Got decorator view ", toplevel->title);
        auto id_str = std::string(toplevel->title).substr(gtk_decorator_prefix.length());
        auto id = std::stoul(id_str.c_str());

        wayfire_view target;
        for (auto& v : wf::get_core().get_all_views())
        {
            if (v->get_id() == id)
            {
                target = v;
            }
        }

        if (!target)
        {
            LOGI("View is gone already?");
            wlr_xdg_toplevel_send_close(toplevel);
            return;
        }

        if (!target->toplevel())
        {
            LOGI("View does not support toplevel interface?");
            wlr_xdg_toplevel_send_close(toplevel);
            return;
        }

        ev->use_default_implementation = false;

        auto data = target->toplevel()->get_data_safe<gtk3_toplevel_custom_data>();

        auto decoration_root_node = std::make_shared<wf::scene::translation_node_t>();
        decoration_root_node->set_offset({-margin_left, -margin_top});
        auto mask_node = std::make_shared<gtk3_mask_node_t>();
        decoration_root_node->set_children_list({mask_node});

        auto deco_surf = std::make_shared<wf::scene::wlr_surface_node_t>(toplevel->base->surface, false);
        data->decoration = std::make_shared<gtk3_decoration_object_t>(toplevel, deco_surf, mask_node);
        mask_node->set_children_list({deco_surf});
  //      decoration_root_node->set_children_list({deco_surf});
        wf::scene::add_back(target->get_surface_root_node(), decoration_root_node);

        target->toplevel()->connect(&on_object_ready);
        target->set_decoration(std::make_unique<gtk3_frame_t>());
    };

    wf::signal::connection_t<wf::view_mapped_signal> on_mapped = [=] (wf::view_mapped_signal *ev)
    {
        LOGI("Need decoration for ", ev->view);
        if (decorator_resource)
        {
            wf_decorator_manager_send_create_new_decoration(decorator_resource, ev->view->get_id());
        }
    };

    wf::signal::connection_t<wf::txn::new_transaction_signal> on_new_tx = [=] (
        wf::txn::new_transaction_signal *ev)
    {
        auto objs = ev->tx->get_objects();
        for (auto& obj : objs)
        {
            if (auto toplvl = dynamic_cast<wf::toplevel_t*>(obj.get()))
            {
                auto deco = toplvl->get_data_safe<gtk3_toplevel_custom_data>();
                if (deco->decoration == nullptr)
                {
                    return;
                }

                ev->tx->add_object(deco->decoration);
                deco->decoration->set_pending_size(wf::dimensions(toplvl->pending().geometry));
            }
        }
    };

    wf::signal::connection_t<wf::txn::object_ready_signal> on_object_ready = [=] (wf::txn::object_ready_signal *ev)
    {
        auto toplvl = dynamic_cast<wf::toplevel_t*>(ev->self);
        auto deco = toplvl->get_data_safe<gtk3_toplevel_custom_data>();
        wf::dassert(deco != nullptr, "obj ready for non-decorated toplevel??");
        deco->decoration->set_final_size(wf::dimensions(toplvl->committed().geometry));
    };

  public:
    void init() override
    {
        decorator_global = wl_global_create(wf::get_core().display,
            &wf_decorator_manager_interface,
            1, NULL, bind_decorator);

        wf::get_core().connect(&on_mapped);
        wf::get_core().connect(&on_new_xdg_surface);
        wf::get_core().tx_manager->connect(&on_new_tx);
    }
};

DECLARE_WAYFIRE_PLUGIN(gtk3_decoration_plugin);
