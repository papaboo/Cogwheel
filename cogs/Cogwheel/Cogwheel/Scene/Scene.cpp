// Cogwheel scene root.
// ---------------------------------------------------------------------------
// Copyright (C) 2015, Cogwheel. See AUTHORS.txt for authors
//
// This program is open source and distributed under the New BSD License. See
// LICENSE.txt for more detail.
// ---------------------------------------------------------------------------

#include <Cogwheel/Scene/Scene.h>

#include <assert.h>

namespace Cogwheel {
namespace Scene {

Scenes::UIDGenerator Scenes::m_UID_generator = UIDGenerator(0u);
Scenes::Scene* Scenes::m_scenes = nullptr;

void Scenes::allocate(unsigned int capacity) {
    if (is_allocated())
        return;

    m_UID_generator = UIDGenerator(capacity);
    capacity = m_UID_generator.capacity();

    m_scenes = new Scene[capacity];
    
    // Allocate dummy element at 0.
    m_scenes[0].name = "Dummy Scene";
    m_scenes[0].root_node = SceneNodes::UID::invalid_UID();
    m_scenes[0].background_color = Math::RGB::black();
    m_scenes[0].environment_map = Assets::Images::UID::invalid_UID();
    m_scenes[0].environment_cdf = Assets::Images::UID::invalid_UID();
}

void Scenes::deallocate() {
    if (!is_allocated())
        return;

    m_UID_generator = UIDGenerator(0u);
    delete[] m_scenes; m_scenes = nullptr;
}

void Scenes::reserve(unsigned int new_capacity) {
    unsigned int old_capacity = capacity();
    m_UID_generator.reserve(new_capacity);
    reserve_scene_data(m_UID_generator.capacity(), old_capacity);
}

template <typename T>
static inline T* resize_and_copy_array(T* old_array, unsigned int new_capacity, unsigned int copyable_elements) {
    T* new_array = new T[new_capacity];
    std::copy(old_array, old_array + copyable_elements, new_array);
    delete[] old_array;
    return new_array;
}

void Scenes::reserve_scene_data(unsigned int new_capacity, unsigned int old_capacity) {
    assert(m_scenes != nullptr);

    const unsigned int copyable_elements = new_capacity < old_capacity ? new_capacity : old_capacity;

    m_scenes = resize_and_copy_array(m_scenes, new_capacity, copyable_elements);
}

Scenes::UID Scenes::create(const std::string& name, SceneNodes::UID root, Math::RGB background_color) {
    assert(m_scenes != nullptr);

    unsigned int old_capacity = m_UID_generator.capacity();
    UID id = m_UID_generator.generate();
    if (old_capacity != m_UID_generator.capacity())
        // The capacity has changed and the size of all arrays need to be adjusted.
        reserve_scene_data(m_UID_generator.capacity(), old_capacity);

    m_scenes[id].name = name;
    m_scenes[id].root_node = root;
    m_scenes[id].background_color = background_color;
    m_scenes[id].environment_map = Assets::Images::UID::invalid_UID();
    m_scenes[id].environment_cdf = Assets::Images::UID::invalid_UID();

    return id;
}

Scenes::UID Scenes::create(const std::string& name, SceneNodes::UID root, Assets::Images::UID environment_map, Assets::Images::UID environment_cdf) {
    assert(m_scenes != nullptr);

    unsigned int old_capacity = m_UID_generator.capacity();
    UID id = m_UID_generator.generate();
    if (old_capacity != m_UID_generator.capacity())
        // The capacity has changed and the size of all arrays need to be adjusted.
        reserve_scene_data(m_UID_generator.capacity(), old_capacity);

    m_scenes[id].name = name;
    m_scenes[id].root_node = root;
    m_scenes[id].background_color = Math::RGB::black();
    m_scenes[id].environment_map = environment_map;
    m_scenes[id].environment_cdf = environment_cdf;

    return id;
}


} // NS Scene
} // NS Cogwheel
