#include "region_tree.h"

region_t* region_t::do_find(int x, int y, unsigned int flag)
{
    if (!contain(x, y)) return NULL;
    if (!has_children()) return (flag & RETURN_SELF)? this: NULL;

    int mid_x = left_ + width_ / 2;
    int mid_y = top_ + height_ / 2;
    int idx;
    if (x < mid_x)
    {
        idx = (y < mid_y)? 0: 2;
    }
    else
    {
        idx = (y < mid_y)? 1: 3;
    }

    if (flag & RECURSE)
    {
        return children_[idx].do_find(x, y, flag);
    }

    return &children_[idx];
}

int region_t::quarter()
{
    if (has_children()) return -1;
    children_ = new region_t[CHILDREN_SIZE];

    memset(children_, 0, sizeof(region_t)*CHILDREN_SIZE);
    int left_w = width_/2;
    int right_w = width_-left_w;
    int top_h = height_/2;
    int bottom_h = height_-top_h;

    for (int i = 0; i < CHILDREN_SIZE; ++i)
    {
        if (0 == i%2)
        {
            children_[i].left_ = left_;
            children_[i].width_ = left_w;
        }
        else
        {
            children_[i].left_ = left_ + left_w;
            children_[i].width_ = right_w;
        }

        if (i < CHILDREN_SIZE/2)
        {
            children_[i].top_ = top_;
            children_[i].height_ = top_h;
        }
        else
        {
            children_[i].top_ = top_ + top_h;
            children_[i].height_ = bottom_h;
        }
    }

    // move all nodes to children
    bool matched;
    deque_node_t* next_node;
    for (deque_node_t* i = head_; i != NULL; )
    {
        next_node = i->next_;
        matched = false;
        for (int j = 0; j < CHILDREN_SIZE; ++j)
        {
            if (children_[j].contain(i->data_->x(), i->data_->y()))
            {
                matched = true;
                children_[j].prepend(*i);
                ++children_[j].num_;
                break;
            }
        }

        if (!matched)
        {
            // FATAL: unknown node, delete it
            delete i;
            i = NULL;
        }

        i = next_node;
    }

    head_ = NULL;
    return 0;
}

void region_t::prepend(deque_node_t& node)
{
    if (head_)
    {
        head_->prev_ = &node;
    }

    node.prev_ = NULL;
    node.next_ = head_;
    head_ = &node;
}

region_tree_t::~region_tree_t()
{
    if (root_)
    {
        clear(*root_);
        delete root_;
        root_ = NULL;
    }
}

void region_tree_t::init( int width, int height, int max_region_data_num, int min_w, int min_h )
{
    if (root_)
    {
        delete root_;
        root_ = NULL;
    }

    width_ = width;
    height_ = height;
    max_region_data_num_ = max_region_data_num;
    min_width_ = min_w;
    min_height_ = min_h;

    root_ = new region_t;
    memset(root_, 0, sizeof(*root_));
    root_->width_ = width;
    root_->height_ = height;
}

void region_tree_t::get_neighbours( int x, int y, overlap_detector_t is_neaby_region, neighbour_filter_t is_neighbour, entities_t* out )
{
    if (root_)
    {
        get_neighbours(*root_, x, y, is_neaby_region, is_neighbour, out);
    }
}

int region_tree_t::move( game_entity_t* p, int x, int y, overlap_detector_t is_overlap, move_events_t* evts )
{
    if (NULL == root_) return -1;
    if (NULL == p) return -1;

    // move to new node if necessary
    region_t* src_zone = root_->find_region(p->x(), p->y());
    if (NULL == src_zone)
    {
        return -1;
    }

    region_t* dst_zone = root_->find_region(x, y);
    if (NULL == dst_zone)
    {
        return -1;
    }

    if (evts)
    {
        move(*root_, p, x, y, REVT_LEAVE|REVT_ENTER, is_overlap, evts);
    }

    p->set_position(x, y);

    if (src_zone != dst_zone)
    {
        // move node to dstzone
        internal_node_t* cut = remove(p, *src_zone);
        if (NULL == cut)
        {
            return -1;
        }

        int ret = move_to_zone(*cut, *dst_zone);
        if (ret < 0)
        {
            return ret;
        }
    }

    return 0;
}

int region_tree_t::remove( game_entity_t* p )
{
    if (NULL == root_) return -1;
    internal_node_t* node = remove(p, *root_);
    if (node)
    {
        delete node;
        return 0;
    }

    return -1;
}

region_tree_t::internal_node_t* region_tree_t::remove( game_entity_t* p, region_t& r )
{
    if (NULL == p) return NULL;
    internal_node_t* remove_node;
    region_t* child = r.get_child(p->x(), p->y());
    if (child)
    {
        remove_node = remove(p, *child);
        if (remove_node)
        {
            --r.num_;
        }

        return remove_node;
    }

    // search for p
    for (internal_node_t* i = r.head_; i != NULL; i = i->next_)
    {
        if (i->data_ == p)
        {
            // find the node
            if (i->prev_)
            {
                i->prev_->next_ = i->next_;
                if (i->next_)
                {
                    i->next_->prev_ = i->prev_;
                }
            }
            else
            {
                // i is head
                r.head_ = i->next_;
                if (i->next_)
                {
                    i->next_->prev_ = NULL;
                }
            }

            --r.num_;
            return i;
        }
    }

    return NULL;
}

void region_tree_t::clear( region_t& r )
{
    if (r.has_children())
    {
        for (int i = 0; i < region_t::CHILDREN_SIZE; ++i)
        {
            clear(r.children_[i]);
        }

        delete []r.children_;
        r.children_ = NULL;
    }

    internal_node_t *n;
    while (NULL != r.head_)
    {
        n = r.head_;
        delete n;
        r.head_ = r.head_->next_;
    }

    r.num_ = 0;
}

void region_tree_t::move( region_t& region, game_entity_t* p, 
                         int dx, int dy, unsigned int event, 
                         overlap_detector_t& is_overlap, move_events_t* evts )
{
    if (!region.has_children())
    {
        for (internal_node_t* i = region.head_; i != NULL; i = i->next_)
        {
            if (i->data_ != p)
            {
                evts->push_back(move_event(event, i->data_));
            }
        }

        return;
    }

    unsigned int new_event;
    for (int i = 0; i < region_t::CHILDREN_SIZE; ++i)
    {
        new_event = 0;
        if ((event & REVT_LEAVE) && is_overlap.func_(p->x(), p->y(), region.children_[i], is_overlap.param_))
        {
            new_event |= REVT_LEAVE;
        }

        if ((event & REVT_ENTER) && is_overlap.func_(dx, dy, region.children_[i], is_overlap.param_))
        {
            new_event |= REVT_ENTER;
        }

        if (new_event)
        {
            move(region.children_[i], p, dx, dy, new_event, is_overlap, evts);
        }
    }
}

void region_tree_t::get_neighbours( region_t& region, int dx, int dy, 
                                   overlap_detector_t& is_neaby_region, 
                                   neighbour_filter_t& is_neighbour, 
                                   entities_t* out )
{
    if (out && !region.has_children())
    {
        for (internal_node_t* i = region.head_; i != NULL; i = i->next_)
        {
            if (is_neighbour.func_(*i->data_, is_neighbour.param_))
            {
                out->push_back(i->data_);
            }
        }

        return;
    }

    for (int i = 0; i < region_t::CHILDREN_SIZE; ++i)
    {
        if (is_neaby_region.func_(dx, dy, region.children_[i], is_neighbour.param_))
        {
            get_neighbours(region.children_[i], dx, dy, is_neaby_region, is_neighbour, out);
        }
    }
}

int region_tree_t::insert( game_entity_t* p, region_t& r )
{
    if (need_quarter(r))
    {
        r.quarter();
    }

    if (r.has_children())
    {
        region_t* child = r.get_child(p->x(), p->y());
        if (NULL == child) return -1;
        int ret = insert(p, *child);
        if (0 == ret)
        {
            ++r.num_;
        }

        return ret;
    }

    internal_node_t* d = new internal_node_t;
    if (NULL == d) return -1;
    d->data_ = p;
    r.prepend(*d);
    ++r.num_;
    return 0;
}

int region_tree_t::move_to_zone( internal_node_t& d, region_t& r )
{
    int x = d.data_->x();
    int y = d.data_->y();
    if (!r.contain(x, y))
    {
        return -1;
    }

    if (need_quarter(r))
    {
        r.quarter();
    }

    if (r.has_children())
    {
        region_t* child = r.get_child(x, y);
        if (NULL == child) return -1;
        int ret = move_to_zone(d, *child);
        if (0 == ret)
        {
            ++r.num_;
        }

        return ret;
    }

    r.prepend(d);
    ++r.num_;
    return 0;
}

bool region_tree_t::need_quarter( const region_t& r ) const
{
    return (int)r.num_ >= max_region_data_num_
        && r.width_/2 >= min_width_
        && r.height_/2 >= min_height_
        && !r.has_children();
}
