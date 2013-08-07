#ifndef _REGION_TREE_H_
#define _REGION_TREE_H_

#include <string.h>
#include <new>
#include <assert.h>
#include <vector>
#include "entity.h"
#include "func_tuple.h"

struct region_t
{
    // all entities linked by deque
    struct deque_node_t
    {
        deque_node_t():
            prev_(NULL),
            next_(NULL),
            data_(NULL)
        {
        }

        ~deque_node_t()
        {
            prev_ = NULL;
            next_ = NULL;
            data_ = NULL;
        }

        deque_node_t* prev_;
        deque_node_t* next_;
        game_entity_t* data_;
    };

    enum
    {
        CHILDREN_SIZE = 4,
    };

    enum
    {
        RECURSE = 0x01,
        RETURN_SELF = 0x02,
    };

    int left_;
    int top_;
    int width_;
    int height_;
    region_t* children_;
    size_t num_;
    deque_node_t* head_;

    bool has_children() const { return NULL != children_; }

    bool contain(int x, int y) const
    {
        return x >= left_ && x < left_ + width_
            && y >= top_ && y < top_ + height_;
    }

    region_t* get_child(int x, int y)
    {
        return do_find(x, y, 0);
    }

    region_t* find_region(int x, int y)
    {
        return do_find(x, y, RECURSE|RETURN_SELF);
    }

    region_t* do_find(int x, int y, unsigned int flag);

    int quarter();

    void prepend(deque_node_t& node);
};

class region_tree_t
{
public:
    enum region_event_t
    {
        REVT_NONE = 0,
        REVT_LEAVE = 0x01,
        REVT_ENTER = 0x02,
        REVT_STAY = REVT_LEAVE|REVT_ENTER,
    };

    struct move_event
    {
        move_event(unsigned int evt, game_entity_t* e):
            evt_(evt), entity_(e)
        {
        }
        unsigned int evt_;
        game_entity_t* entity_;
    };

    typedef region_t::deque_node_t internal_node_t;
    typedef std::vector<game_entity_t*> entities_t;
    typedef std::vector<move_event> move_events_t;
    typedef func_tuple_t<bool(*)(int, int, const region_t&, void*)> overlap_detector_t;
    typedef func_tuple_t<bool(*)(const game_entity_t&, void*)> neighbour_filter_t;
    
    region_tree_t()
    {
        root_ = NULL;
        init(0, 0, 0, 0, 0);
    }

    region_tree_t(int width, int height, int max_region_data_num, int min_w, int min_h)
    {
        root_ = NULL;
        init(width, height, max_region_data_num, min_w, min_h);
    }

    ~region_tree_t();

    void init(int width, int height, int max_region_data_num, int min_w, int min_h);

    int insert(game_entity_t* p)
    {
        if (NULL == root_) return -1;
        return insert(p, *root_);
    }

    void get_neighbours(int x, int y, overlap_detector_t is_neaby_region, neighbour_filter_t is_neighbour, entities_t* out);
    // move entity p to (x,y), return 0 if succ, -1 if error
    int move(game_entity_t* p, int x, int y, overlap_detector_t is_overlap, move_events_t* evts);

    int remove(game_entity_t* p);

    region_t* get_root() { return root_; }
private:
    void clear(region_t& r);

    void move(region_t& region, game_entity_t* p, int dx, int dy, unsigned int event, 
        overlap_detector_t& is_overlap, move_events_t* evts);

    void get_neighbours(region_t& region, int dx, int dy, overlap_detector_t& is_neaby_region, neighbour_filter_t& is_neighbour, entities_t* out);

    internal_node_t* remove(game_entity_t* p, region_t& r);

    int insert(game_entity_t* p, region_t& r);

    int move_to_zone(internal_node_t& d, region_t& r);

    bool need_quarter(const region_t& r) const;

    region_t* root_;
    int width_;
    int height_;
    int max_region_data_num_;
    int min_width_;
    int min_height_;
};


#endif
