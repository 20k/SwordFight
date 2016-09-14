#ifndef TROMBONE_MANAGER_HPP_INCLUDED
#define TROMBONE_MANAGER_HPP_INCLUDED

struct engine;
struct fighter;
struct object_context;
struct objects_container;

struct trombone_manager
{
    void init(object_context* _ctx);

    void tick(engine& window, fighter* my_fight);
    void play(fighter* my_fight);

    void set_active(bool active);

    bool is_active = false;

    int tone = 0;
    object_context* ctx = nullptr;
    objects_container* trombone = nullptr;

    static constexpr int max_tones = 13;
};

#endif // TROMBONE_MANAGER_HPP_INCLUDED
