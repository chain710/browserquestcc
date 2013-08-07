#include "../region_tree.h"
#include "gtest/gtest.h"
#include <set>
#include <math.h>
using namespace std;

struct overlap_checker
{
    int radius_;
};

class event_handler
{
public:
    set<int> stay_ids_;
    set<int> leave_ids_;
    set<int> enter_ids_;
    void reset()
    {
        stay_ids_.clear();
        leave_ids_.clear();
        enter_ids_.clear();
    }

    void from_events(region_tree_t::move_events_t& evts)
    {
        for (region_tree_t::move_events_t::iterator i = evts.begin(); i != evts.end(); ++i)
        {
            unsigned int evt = i->evt_;
            set<int>* pset = NULL;
            if (evt == region_tree_t::REVT_LEAVE)
            {
                pset = &leave_ids_;
            }
            else if (evt == region_tree_t::REVT_ENTER)
            {
                pset = &enter_ids_;
            }
            else if (evt == (region_tree_t::REVT_STAY))
            {
                pset = &stay_ids_;
            }

            ASSERT_TRUE(pset != NULL);
            EXPECT_TRUE(pset->find(i->entity_->id()) == pset->end());
            pset->insert(i->entity_->id());
        }
    }
};

bool is_overlap(int x, int y, const region_t& region, void* up)
{
    // rect overlap
    overlap_checker* c = (overlap_checker*)up;

    float rmx = region.left_ + (float)(region.width_)/2;
    float rmy = region.top_ + (float)(region.height_)/2;
    float w = c->radius_+(float)(region.width_)/2;
    float h = c->radius_+(float)(region.height_)/2;
    return (fabs(rmx-x) <= w && fabs(rmy-y) <= h);
}

TEST(quad_tree, maintest)
{
    const int first_size = 10;
    const int min_w = 1;
    const int min_h = 1;

    game_entity_t first[first_size];
    
    int ret;
    region_tree_t tree(11, 11, 3, min_w, min_h);
    for (int i = 0; i < first_size; ++i)
    {
        first[i].set_id(i);
        first[i].set_position(0, 0);
        ret = tree.insert(&first[i]);
        EXPECT_EQ(0, ret);
    }

    region_t *root = tree.get_root();
    EXPECT_EQ((int)root->num_, first_size);

    region_t *dst = root->find_region(0, 0);
    EXPECT_NE(root, dst);
    EXPECT_EQ(dst->width_, min_w);
    EXPECT_EQ(dst->height_, min_h);
    EXPECT_EQ((int)dst->num_, first_size);

    for (int i = 0; i < first_size; ++i)
    {
        ret = tree.remove(&first[i]);
        EXPECT_EQ(0, ret);
    }

    EXPECT_TRUE(root->head_ == NULL);
    EXPECT_EQ((int)root->num_, 0);
    EXPECT_EQ((int)dst->num_, 0);
    EXPECT_TRUE(dst->head_ == NULL);
}

TEST(quad_tree, insert)
{
    const int first_size = 5;
    const int min_w = 1;
    const int min_h = 1;

    game_entity_t first[first_size];
    first[0].set_position(0, 0);
    first[1].set_position(10, 1);
    first[2].set_position(3, 9);
    first[3].set_position(10, 9);
    first[4].set_position(9, 9);
    int ret;
    region_tree_t tree(11, 11, 3, min_w, min_h);
    for (int i = 0; i < first_size; ++i)
    {
        ret = tree.insert(&first[i]);
        EXPECT_EQ(0, ret);
    }

    region_t *root = tree.get_root();
    EXPECT_EQ((int)root->num_, first_size);
    EXPECT_EQ((int)root->children_[0].num_, 1);
    EXPECT_EQ((int)root->children_[1].num_, 1);
    EXPECT_EQ((int)root->children_[2].num_, 1);
    EXPECT_EQ((int)root->children_[3].num_, 2);
}

TEST(quad_tree, test_overlap)
{
    const int min_w = 3;
    const int min_h = 3;
    region_tree_t tree(11, 11, 1, min_w, min_h);

    const int first_size = 4;
    game_entity_t first[first_size];
    first[0].set_position(0, 0);
    first[1].set_position(0, 1);
    first[2].set_position(5, 1);
    first[3].set_position(7, 7);

    int ret;

    for (int i = 0; i < first_size; ++i)
    {
        first[i].set_id(i);
        ret = tree.insert(&first[i]);
        EXPECT_EQ(0, ret);
    }

    overlap_checker up;
    up.radius_ = 3;
    region_tree_t::overlap_detector_t ifoverlap;
    ifoverlap.func_ = is_overlap;
    ifoverlap.param_ = &up;

    event_handler evthandler;
    region_tree_t::move_events_t evts;

    tree.move(&first[0], 1, 1, ifoverlap, &evts);
    evthandler.from_events(evts);

    // should stay
    EXPECT_TRUE(evthandler.enter_ids_.empty());
    EXPECT_TRUE(evthandler.leave_ids_.empty());
    EXPECT_EQ((int)evthandler.stay_ids_.size(), 1);
    EXPECT_TRUE(evthandler.stay_ids_.find(1) != evthandler.stay_ids_.end());

    evthandler.reset();
    evts.clear();
    tree.move(&first[0], 2, 1, ifoverlap, &evts);
    evthandler.from_events(evts);

    EXPECT_EQ((int)evthandler.enter_ids_.size(), 1);
    EXPECT_TRUE(evthandler.enter_ids_.find(2) != evthandler.enter_ids_.end());
    EXPECT_TRUE(evthandler.leave_ids_.empty());
    EXPECT_TRUE(evthandler.stay_ids_.size() == 1);
    EXPECT_TRUE(evthandler.stay_ids_.find(1) != evthandler.stay_ids_.end());

    evthandler.reset();
    evts.clear();
    tree.move(&first[0], 9, 6, ifoverlap, &evts);
    evthandler.from_events(evts);
    EXPECT_EQ((int)evthandler.enter_ids_.size(), 1);
    EXPECT_TRUE(evthandler.enter_ids_.find(3) != evthandler.enter_ids_.end());
    EXPECT_EQ((int)evthandler.leave_ids_.size(), 1);
    EXPECT_TRUE(evthandler.leave_ids_.find(1) != evthandler.leave_ids_.end());
    EXPECT_TRUE(evthandler.stay_ids_.size() == 1);
    EXPECT_TRUE(evthandler.stay_ids_.find(2) != evthandler.stay_ids_.end());
}
