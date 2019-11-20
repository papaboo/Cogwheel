// Bifrost image.
// ---------------------------------------------------------------------------
// Copyright (C) Bifrost. See AUTHORS.txt for authors.
//
// This program is open source and distributed under the New BSD License.
// See LICENSE.txt for more detail.
// ---------------------------------------------------------------------------

#include <Bifrost/Assets/Image.h>

#include <assert.h>

using namespace Bifrost::Math;

namespace Bifrost {
namespace Assets {

Images::UIDGenerator Images::m_UID_generator = UIDGenerator(0u);
Images::MetaInfo* Images::m_metainfo = nullptr;
Images::PixelData* Images::m_pixels = nullptr;
Core::ChangeSet<Images::Changes, Images::UID> Images::m_changes;

void Images::allocate(unsigned int capacity) {
    if (is_allocated())
        return;

    m_UID_generator = UIDGenerator(capacity);
    capacity = m_UID_generator.capacity();

    m_metainfo = new MetaInfo[capacity];
    m_pixels = new PixelData[capacity];
    m_changes = Core::ChangeSet<Changes, UID>(capacity);

    // Allocate dummy element at 0.
    MetaInfo info = { "Dummy image", 0u, 0u, 0u, 0u, PixelFormat::Unknown };
    m_metainfo[0] = info;
    m_pixels[0] = nullptr;
}

void Images::deallocate() {
    if (!is_allocated())
        return;

    m_UID_generator = UIDGenerator(0u);
    delete[] m_metainfo; m_metainfo = nullptr;
    delete[] m_pixels; m_pixels = nullptr;
    m_changes.resize(0);
}

template <typename T>
static inline T* resize_and_copy_array(T* old_array, unsigned int new_capacity, unsigned int copyable_elements) {
    T* new_array = new T[new_capacity];
    std::copy(old_array, old_array + copyable_elements, new_array);
    delete[] old_array;
    return new_array;
}

void Images::reserve_image_data(unsigned int new_capacity, unsigned int old_capacity) {
    assert(m_metainfo != nullptr);
    assert(m_pixels != nullptr);

    const unsigned int copyable_elements = new_capacity < old_capacity ? new_capacity : old_capacity;
    m_metainfo = resize_and_copy_array(m_metainfo, new_capacity, copyable_elements);
    m_pixels = resize_and_copy_array(m_pixels, new_capacity, copyable_elements);
    m_changes.resize(new_capacity);
}

void Images::reserve(unsigned int new_capacity) {
    unsigned int old_capacity = capacity();
    m_UID_generator.reserve(new_capacity);
    reserve_image_data(m_UID_generator.capacity(), old_capacity);
}

bool Images::has(Images::UID image_ID) {
    return m_UID_generator.has(image_ID) && m_changes.get_changes(image_ID) != Change::Destroyed;
}

static inline Images::PixelData allocate_pixels(PixelFormat format, unsigned int pixel_count) {
    switch (format) {
    case PixelFormat::Alpha8:
    case PixelFormat::Intensity8:
        return new unsigned char[pixel_count];
    case PixelFormat::RGB24:
        return new unsigned char[3 * pixel_count];
    case PixelFormat::RGBA32:
        return new unsigned char[4 * pixel_count];
    case PixelFormat::Intensity_Float:
        return new float[pixel_count];
    case PixelFormat::RGB_Float:
        return new RGB[pixel_count];
    case PixelFormat::RGBA_Float:
        return new RGBA[pixel_count];
    case PixelFormat::Unknown:
        return nullptr;
    }
    return nullptr;
}

static inline void deallocate_pixels(PixelFormat format, Images::PixelData data) {
    switch (format) {
    case PixelFormat::Alpha8:
    case PixelFormat::Intensity8:
    case PixelFormat::RGB24:
    case PixelFormat::RGBA32:
        delete[] (unsigned char*)data;
        break;
    case PixelFormat::Intensity_Float:
        delete[] (float*)data;
        break;
    case PixelFormat::RGB_Float:
        delete[] (RGB*)data;
        break;
    case PixelFormat::RGBA_Float:
        delete[] (RGBA*)data;
        break;
    case PixelFormat::Unknown:
        printf("WARNING: Deallocating unknown pixel format.\n");
    }
}

Images::UID Images::create3D(const std::string& name, PixelFormat format, float gamma, Vector3ui size, unsigned int mipmap_count) {
    assert(m_metainfo != nullptr);
    assert(m_pixels != nullptr);
    assert(mipmap_count > 0u);

    unsigned int old_capacity = m_UID_generator.capacity();
    UID id = m_UID_generator.generate();
    if (old_capacity != m_UID_generator.capacity())
        // The capacity has changed and the size of all arrays need to be adjusted.
        reserve_image_data(m_UID_generator.capacity(), old_capacity);

    // Only apply gamma to images that store colors.
    if (format == PixelFormat::Alpha8)
        gamma = 1.0f;

    MetaInfo& metainfo = m_metainfo[id];
    metainfo.name = name;
    metainfo.pixel_format = format;
    metainfo.gamma = gamma;
    metainfo.width = size.x;
    metainfo.height = size.y;
    metainfo.depth = size.z;
    unsigned int total_pixel_count = 0u;
    unsigned int mip_count = 0u;
    while (mip_count != mipmap_count) {
        unsigned int mip_pixel_count = Images::get_width(id, mip_count) * Images::get_height(id, mip_count) * Images::get_depth(id, mip_count);
        total_pixel_count += mip_pixel_count;
        ++mip_count;
        if (mip_pixel_count == 1u)
            break;
    }
    metainfo.mipmap_count = mip_count;
    metainfo.is_mipmapable = false;
    m_pixels[id] = allocate_pixels(format, total_pixel_count);
    m_changes.set_change(id, Change::Created);

    return id;
}

Images::UID Images::create2D(const std::string& name, PixelFormat format, float gamma, Math::Vector2ui size, PixelData& pixels) {
    assert(m_metainfo != nullptr);
    assert(m_pixels != nullptr);

    unsigned int old_capacity = m_UID_generator.capacity();
    UID id = m_UID_generator.generate();
    if (old_capacity != m_UID_generator.capacity())
        // The capacity has changed and the size of all arrays need to be adjusted.
        reserve_image_data(m_UID_generator.capacity(), old_capacity);

    // Only apply gamma to images that store colors.
    if (format == PixelFormat::Alpha8)
        gamma = 1.0f;

    MetaInfo& metainfo = m_metainfo[id];
    metainfo.name = name;
    metainfo.pixel_format = format;
    metainfo.gamma = gamma;
    metainfo.width = size.x;
    metainfo.height = size.y;
    metainfo.depth = 1u;

    metainfo.mipmap_count = 1u;
    metainfo.is_mipmapable = false;
    m_pixels[id] = pixels; pixels = nullptr; // Take ownership of pixels.
    m_changes.set_change(id, Change::Created);

    return id;
}

void Images::destroy(Images::UID image_ID) {
    if (m_UID_generator.erase(image_ID)) {
        deallocate_pixels(m_metainfo[image_ID].pixel_format, m_pixels[image_ID]);
        m_pixels[image_ID] = nullptr;
        m_changes.set_change(image_ID, Change::Destroyed);
    }
}

void Images::set_mipmapable(Images::UID image_ID, bool value) { 
    // Only set as mipmapable if no mipmaps exist.
    value &= m_metainfo[image_ID].mipmap_count == 1;

    if (m_metainfo[image_ID].is_mipmapable == value)
        return;

    m_metainfo[image_ID].is_mipmapable = value;

    m_changes.add_change(image_ID, Change::Mipmapable);
}

Images::PixelData Images::get_pixels(Images::UID image_ID, int mipmap_level) {
    char* pixel_data = (char*)m_pixels[image_ID];
    int bytes_pr_pixel = size_of(get_pixel_format(image_ID));
    for (int l = 0; l < mipmap_level; ++l) {
        pixel_data += get_pixel_count(image_ID, l) * bytes_pr_pixel;
    }
    return pixel_data;
}

static RGBA get_nonlinear_pixel(Images::PixelData pixels, PixelFormat format, unsigned int index) {
    switch (format) {
    case PixelFormat::Alpha8: {
        float alpha = ((unsigned char*)pixels)[index] / 255.0f;
        return RGBA(1.0f, 1.0f, 1.0f, alpha);
    }
    case PixelFormat::Intensity8: {
        float i = ((unsigned char*)pixels)[index] / 255.0f;
        return RGBA(i, i, i, 1.0f);
    }
    case PixelFormat::RGB24: {
        unsigned char* pixel = ((unsigned char*)pixels) + index * 3;
        return RGBA(pixel[0] / 255.0f, pixel[1] / 255.0f, pixel[2] / 255.0f, 1.0f);
    }
    case PixelFormat::RGBA32: {
        unsigned char* pixel = ((unsigned char*)pixels) + index * 4;
        return RGBA(pixel[0] / 255.0f, pixel[1] / 255.0f, pixel[2] / 255.0f, pixel[3] / 255.0f);
    }
    case PixelFormat::Intensity_Float: {
        float value = ((float*)pixels)[index];
        return RGBA(value, value, value, 1.0f);
    }
    case PixelFormat::RGB_Float: {
        RGB pixel = ((RGB*)pixels)[index];
        return RGBA(pixel.r, pixel.g, pixel.b, 1.0f);
    }
    case PixelFormat::RGBA_Float: {
        return ((RGBA*)pixels)[index];
    }
    case PixelFormat::Unknown:
        return RGBA::red();
    }
    return RGBA::red();
}

static inline RGBA get_linear_pixel(Images::UID image_ID, unsigned int index) {
    Images::PixelData pixels = Images::get_pixels(image_ID);
    PixelFormat format = Images::get_pixel_format(image_ID);
    RGBA nonlinear_color = get_nonlinear_pixel(pixels, format, index);
    return gammacorrect(nonlinear_color, Images::get_gamma(image_ID));
}

RGBA Images::get_pixel(Images::UID image_ID, unsigned int index, unsigned int mipmap_level) {
    assert(index < Images::get_pixel_count(image_ID, mipmap_level));

    while (mipmap_level)
        index += Images::get_width(image_ID, --mipmap_level);
    return get_linear_pixel(image_ID, index);
}

RGBA Images::get_pixel(Images::UID image_ID, Vector2ui index, unsigned int mipmap_level) {
    assert(index.x < Images::get_width(image_ID, mipmap_level));
    assert(index.y < Images::get_height(image_ID, mipmap_level));

    Image image = image_ID;
    unsigned int pixel_index = index.x + image.get_width(mipmap_level) * index.y;

    while (mipmap_level) {
        --mipmap_level;
        pixel_index += image.get_width(mipmap_level) * image.get_height(mipmap_level);
    }
    return get_linear_pixel(image_ID, pixel_index);
}

RGBA Images::get_pixel(Images::UID image_ID, Vector3ui index, unsigned int mipmap_level) {
    assert(index.x < Images::get_width(image_ID, mipmap_level));
    assert(index.y < Images::get_height(image_ID, mipmap_level));
    assert(index.z < Images::get_depth(image_ID, mipmap_level));

    Image image = image_ID;
    unsigned int pixel_index = index.x + image.get_width(mipmap_level) * (index.y + image.get_height(mipmap_level) * index.z);
    while (mipmap_level) {
        --mipmap_level;
        pixel_index += image.get_width(mipmap_level) * image.get_height(mipmap_level) * image.get_depth(mipmap_level);
    }
    return get_linear_pixel(image_ID, pixel_index);
}

static void set_linear_pixel(Images::PixelData pixels, PixelFormat pixel_format, unsigned int index, RGBA color, float gamma) {
    color = gammacorrect(color, 1.0f / gamma);
    switch (pixel_format) {
    case PixelFormat::Alpha8: {
        unsigned char* pixel = ((unsigned char*)pixels) + index;
        pixel[0] = unsigned char(clamp(color.a * 255.0f + 0.5f, 0.0f, 255.0f));
        break;
    }
    case PixelFormat::Intensity8: {
        unsigned char* pixel = ((unsigned char*)pixels) + index;
        pixel[0] = unsigned char(clamp(color.r * 255.0f + 0.5f, 0.0f, 255.0f));
        break;
    }
    case PixelFormat::RGB24: {
        unsigned char* pixel = ((unsigned char*)pixels) + index * 3;
        pixel[0] = unsigned char(clamp(color.r * 255.0f + 0.5f, 0.0f, 255.0f));
        pixel[1] = unsigned char(clamp(color.g * 255.0f + 0.5f, 0.0f, 255.0f));
        pixel[2] = unsigned char(clamp(color.b * 255.0f + 0.5f, 0.0f, 255.0f));
        break;
    }
    case PixelFormat::RGBA32: {
        unsigned char* pixel = ((unsigned char*)pixels) + index * 4;
        pixel[0] = unsigned char(clamp(color.r * 255.0f + 0.5f, 0.0f, 255.0f));
        pixel[1] = unsigned char(clamp(color.g * 255.0f + 0.5f, 0.0f, 255.0f));
        pixel[2] = unsigned char(clamp(color.b * 255.0f + 0.5f, 0.0f, 255.0f));
        pixel[3] = unsigned char(clamp(color.a * 255.0f + 0.5f, 0.0f, 255.0f));
        break;
    }
    case PixelFormat::Intensity_Float: {
        ((float*)pixels)[index] = color.r;
        break;
    }
    case PixelFormat::RGB_Float: {
        ((RGB*)pixels)[index] = color.rgb();
        break;
    }
    case PixelFormat::RGBA_Float: {
        ((RGBA*)pixels)[index] = color;
        break;
    }
    case PixelFormat::Unknown:
        ;
    }
}

static inline void set_linear_pixel(Images::UID image_ID, RGBA color, unsigned int index) {
    Images::PixelData pixels = Images::get_pixels(image_ID);
    PixelFormat format = Images::get_pixel_format(image_ID);
    set_linear_pixel(pixels, format, index, color, Images::get_gamma(image_ID));
}

void Images::set_pixel(Images::UID image_ID, RGBA color, unsigned int index, unsigned int mipmap_level) {
    assert(index < Images::get_pixel_count(image_ID, mipmap_level));

    Image image = image_ID;
    unsigned int width = image.get_width();
    while (mipmap_level)
        index += image.get_width(--mipmap_level);
    set_linear_pixel(image_ID, color, index);
    m_changes.add_change(image_ID, Change::PixelsUpdated);
}

void Images::set_pixel(Images::UID image_ID, RGBA color, Vector2ui index, unsigned int mipmap_level) {
    assert(index.x < Images::get_width(image_ID, mipmap_level));
    assert(index.y < Images::get_height(image_ID, mipmap_level));

    Image image = image_ID;
    unsigned int pixel_index = index.x + image.get_width(mipmap_level) * index.y;
    while (mipmap_level) {
        --mipmap_level;
        pixel_index += image.get_width(mipmap_level) * image.get_height(mipmap_level);
    }
    set_linear_pixel(image_ID, color, pixel_index);
    m_changes.add_change(image_ID, Change::PixelsUpdated);
}

void Images::set_pixel(Images::UID image_ID, RGBA color, Vector3ui index, unsigned int mipmap_level) {
    assert(index.x < Images::get_width(image_ID, mipmap_level));
    assert(index.y < Images::get_height(image_ID, mipmap_level));
    assert(index.z < Images::get_depth(image_ID, mipmap_level));

    Image image = image_ID;
    unsigned int pixel_index = index.x + image.get_width(mipmap_level) * (index.y + image.get_height(mipmap_level) * index.z);
    while (mipmap_level) {
        --mipmap_level;
        pixel_index += image.get_width(mipmap_level) * image.get_height(mipmap_level) * image.get_depth(mipmap_level);
    }
    set_linear_pixel(image_ID, color, pixel_index);
    m_changes.add_change(image_ID, Change::PixelsUpdated);
}

void Images::change_format(Images::UID image_ID, PixelFormat new_format, float new_gamma) {
    Image image = image_ID;
    PixelFormat old_format = image.get_pixel_format();
    float old_gamma = image.get_gamma();

    unsigned int total_pixel_count = image.get_width() * image.get_height() * image.get_depth();
    for (unsigned int m = 1; m < image.get_mipmap_count(); ++m)
        total_pixel_count += image.get_width(m) * image.get_height(m) * image.get_depth(m);

    auto gamma_correct_bytes = [](unsigned char* pixels, unsigned int total_pixel_count, float gamma) {
        for (unsigned int p = 0; p < total_pixel_count; ++p) {
            unsigned char linear_value = pixels[p];
            float non_linear_pixel = pow(linear_value / 255.0f, gamma);
            pixels[p] = unsigned char(clamp(non_linear_pixel * 255.0f + 0.5f, 0.0f, 255.0f));
        }
    };

    if (old_format == PixelFormat::Intensity8 && new_format == PixelFormat::Alpha8) {
        // Gamma correct if intensity values have been gamma corrected.
        // Alpha is not affected by gamma, so new gamma is effectively one.
        if (old_gamma != 1.0f)
            gamma_correct_bytes(image.get_pixels<unsigned char>(), total_pixel_count, old_gamma);
    } else if (old_format == PixelFormat::Alpha8 && new_format == PixelFormat::Intensity8) {
        // Alpha is not affected by gamma, so old gamma is effectively one.
        if (new_gamma != 1.0f)
            gamma_correct_bytes(image.get_pixels<unsigned char>(), total_pixel_count, 1.0f / new_gamma);
    } else {
        // Fallback RGBA pixel copy
        PixelData new_pixels = allocate_pixels(new_format, total_pixel_count);

        // Copy pixels
        if (!has_alpha(old_format) && new_format == PixelFormat::Alpha8) {
            // Copy from RGB channels to alpha.
            int channel_count = Assets::channel_count(old_format);
            float normalizer = 1.0f / channel_count;
            for (unsigned int p = 0; p < total_pixel_count; ++p) {
                RGBA pixel = get_linear_pixel(image_ID, p);
                pixel.a = pixel.r;
                if (channel_count > 0) pixel.a += pixel.g;
                if (channel_count > 1) pixel.a += pixel.b;
                pixel.a *= normalizer;
                set_linear_pixel(new_pixels, new_format, p, pixel, new_gamma);
            }
        } else if (old_format == PixelFormat::Alpha8 && !has_alpha(new_format)) {
            // Copy from alpha to RGB channels.
            for (unsigned int p = 0; p < total_pixel_count; ++p) {
                RGBA pixel = get_linear_pixel(image_ID, p);
                pixel.r = pixel.g = pixel.b = pixel.a;
                set_linear_pixel(new_pixels, new_format, p, pixel, new_gamma);
            }
        } else {
            for (unsigned int p = 0; p < total_pixel_count; ++p) {
                RGBA pixel = get_linear_pixel(image_ID, p);
                set_linear_pixel(new_pixels, new_format, p, pixel, new_gamma);
            }
        }

        deallocate_pixels(old_format, m_pixels[image_ID]);
        m_pixels[image_ID] = new_pixels;
    }

    m_metainfo[image_ID].pixel_format = new_format;
    m_changes.add_change(image_ID, Change::PixelsUpdated);
}


//*****************************************************************************
// Image Utilities
//*****************************************************************************

namespace ImageUtils {

void fill_mipmap_chain(Images::UID image_ID) {
    // assert that depth is 1, since 3D textures are not supported.

    // Future work: Optimize for the most used data formats.
    Image image = image_ID;
    for (unsigned int m = 0; m < image.get_mipmap_count() - 1; ++m) {
        for (unsigned int y = 0; y + 1 < image.get_height(m); y += 2) { // TODO Doesn't work with 1D textures does it?
            for (unsigned int x = 0; x + 1 < image.get_width(m); x += 2) {

                RGBA lower_left = image.get_pixel(Vector2ui(x, y), m);
                RGBA lower_right = image.get_pixel(Vector2ui(x + 1, y), m);
                RGBA upper_left = image.get_pixel(Vector2ui(x, y + 1), m);
                RGBA upper_right = image.get_pixel(Vector2ui(x + 1, y + 1), m);

                RGB new_rgb = (lower_left.rgb() + lower_right.rgb() + upper_left.rgb() + upper_right.rgb()) * 0.25f;
                float new_alpha = (lower_left.a + lower_right.a + upper_left.a + upper_right.a) * 0.25f;
                image.set_pixel(RGBA(new_rgb, new_alpha), Vector2ui(x / 2, y / 2), m + 1);
            }

            // If uneven number of columns, then add the last column into the last column of the next mipmap level.
            if (image.get_width(m) & 0x1) {
                RGBA left = image.get_pixel(Vector2ui(image.get_width(m + 1) - 1, y / 2), m + 1);
                RGBA lower_right = image.get_pixel(Vector2ui(image.get_width(m) - 1, y), m);
                RGBA upper_right = image.get_pixel(Vector2ui(image.get_width(m) - 1, y + 1), m);
                RGB rgb = (left.rgb() * 4.0f + lower_right.rgb() + upper_right.rgb()) / 6.0f;
                float alpha = (left.a * 4.0f + lower_right.a + upper_right.a) / 6.0f;
                image.set_pixel(RGBA(rgb, alpha), Vector2ui((image.get_width(m + 1) - 1), y / 2), m + 1);
            }
        }

        // If uneven number of rows, then add the last row into the last row of the next mipmap level.
        if (image.get_height(m) & 0x1) {
            bool uneven_column_count = image.get_width(m) & 0x1;
            unsigned int regular_columns = image.get_width(m) - (uneven_column_count ? 3u : 0u);
            for (unsigned int x = 0; x < regular_columns; x += 2) {
                RGBA lower = image.get_pixel(Vector2ui(x / 2, image.get_height(m + 1) - 1), m + 1);
                RGBA upper_left = image.get_pixel(Vector2ui(x, image.get_height(m) - 1), m);
                RGBA upper_right = image.get_pixel(Vector2ui(x + 1, image.get_height(m) - 1), m);
                RGB rgb = (lower.rgb() * 4.0f + upper_left.rgb() + upper_right.rgb()) / 6.0f;
                float alpha = (lower.a * 4.0f + upper_left.a + upper_right.a) / 6.0f;
                image.set_pixel(RGBA(rgb, alpha), Vector2ui(x / 2, (image.get_height(m + 1) - 1)), m + 1);
            }

            // If both the row and column count are uneven, then we still need to blend 3 edge pixels into the next mipmap pixel
            if (uneven_column_count) {
                RGBA lower = image.get_pixel(Vector2ui(image.get_width(m + 1) - 1, image.get_height(m + 1) - 1), m + 1);
                RGBA upper_left = image.get_pixel(Vector2ui(image.get_width(m) - 3, image.get_height(m) - 1), m);
                RGBA upper_middle = image.get_pixel(Vector2ui(image.get_width(m) - 2, image.get_height(m) - 1), m);
                RGBA upper_right = image.get_pixel(Vector2ui(image.get_width(m) - 1, image.get_height(m) - 1), m);
                RGB rgb = (lower.rgb() * 6.0f + upper_left.rgb() + upper_middle.rgb() + upper_right.rgb()) / 9.0f;
                float alpha = (lower.a * 6.0f + upper_left.a + upper_middle.a + upper_right.a) / 9.0f;
                image.set_pixel(RGBA(rgb, alpha), Vector2ui(image.get_width(m + 1) - 1, image.get_height(m + 1) - 1), m + 1);
            }
        }
    }
}

void compute_summed_area_table(Images::UID image_ID, RGBA* sat_result) {
    Image img = image_ID;
    unsigned int width = img.get_width(), height = img.get_height();

    // Initialize high precision buffer.
    // TODO Instead of preallocating a huge Vector4d array, two rows (current and previous) would do.
    Vector4d* sat = new Vector4d[width * height];

    auto RGBA_to_vector4d = [](RGBA rgba) -> Vector4d { return Vector4d(rgba.r, rgba.g, rgba.b, rgba.a); };

    // Fill the lower row and left column.
    sat[0] = RGBA_to_vector4d(img.get_pixel(Vector2ui(0, 0)));
    for (unsigned int x = 1; x < width; ++x)
        sat[x] = RGBA_to_vector4d(img.get_pixel(Vector2ui(x, 0))) + sat[x-1];
    for (unsigned int y = 1; y < height; ++y)
        sat[y * width] = RGBA_to_vector4d(img.get_pixel(Vector2ui(0, y))) + sat[(y - 1)  * width];

    for (unsigned int y = 1; y < height; ++y)
        for (unsigned int x = 1; x < width; ++x) {
            Vector4d pixel = RGBA_to_vector4d(img.get_pixel(Vector2ui(x, y)));
            sat[x + y * width] = pixel + sat[x + (y - 1)  * width] + sat[(x - 1) + y  * width] - sat[(x - 1) + (y - 1)  * width];
        }

    for (unsigned int y = 0; y < height; ++y)
        for (unsigned int x = 0; x < width; ++x) {
            Vector4d v = sat[x + y * width];
            sat_result[x + y * width] = { float(v.x), float(v.y), float(v.z), float(v.w) };
        }

    delete[] sat;
}

Image combine_tint_roughness(const Image tint, const Image roughness, int roughness_channel) {
    PixelFormat tint_format = tint.get_pixel_format();
    PixelFormat roughness_format = roughness.get_pixel_format();

    Vector2ui size = { tint.get_width(), tint.get_height() };

    // Handle cases where one of the two textures doesn't exists and the other should be used.
    if (!roughness.exists()) {
        if (has_alpha(tint_format)) {
            assert(tint_format == PixelFormat::RGBA32); // The alternative is float, but float isn't usual for tint.
            Image tint_sans_roughness = Images::create2D(tint.get_name(), PixelFormat::RGBA32, 2.2f, size, tint.get_mipmap_count());
            RGBA32* new_tint_pixels = tint_sans_roughness.get_pixels<RGBA32>();
            memcpy(new_tint_pixels, tint.get_pixels(), size.x * size.y * size_of(tint_format));
            // Set roughness to multiplicative identity.
            for (unsigned int i = 0; i < size.x * size.y; ++i)
                new_tint_pixels[i].a = 255;
            return tint_sans_roughness;
        } else
            return tint;
    }
    
    if (!tint.exists()) {
        if (roughness_format == PixelFormat::Roughness8)
            return roughness;
        else
            return ImageUtils::copy_with_new_format(roughness.get_ID(), PixelFormat::Roughness8, 1.0f, [=](RGBA pixel) -> RGBA {
                float v = pixel[roughness_channel];
                return RGBA(v, v, v, v);
            });
    }

    assert(tint.get_width() == roughness.get_width());
    assert(tint.get_height() == roughness.get_height());
    assert(tint.get_depth() == roughness.get_depth() && tint.get_depth() == 1);

    int mipmap_count = min(tint.get_mipmap_count(), roughness.get_mipmap_count());
    int pixel_count = tint.get_pixel_count();
    for (int m = 1; m < mipmap_count; ++m)
        pixel_count += tint.get_pixel_count(m);

    int chunk_size = 4096;
    int chunk_count = ceil_divide(pixel_count, chunk_size);

    bool tint_is_byte = tint_format == PixelFormat::RGB24 || tint_format == PixelFormat::RGBA32;
    bool roughness_is_byte = roughness_format == PixelFormat::Alpha8 || roughness_format == PixelFormat::Intensity8 || 
        roughness_format == PixelFormat::RGB24 || roughness_format == PixelFormat::RGBA32;
    if (tint_is_byte && roughness_is_byte) {
        int tint_pixel_size = size_of(tint_format);
        int roughness_pixel_size = size_of(roughness_format);

        assert(roughness_format != PixelFormat::Intensity8 || (roughness_format == PixelFormat::Intensity8 && roughness_channel == 0)); // Roughness with PixelFormat::Intensity8 must use channel one for roughness.
        assert(roughness_format != PixelFormat::Alpha8 || (roughness_format == PixelFormat::Alpha8 && roughness_channel == 3)); // Roughness with PixelFormat::Alpha8 must use channel three for roughness.
        assert(roughness_format != PixelFormat::RGB24 || (roughness_format == PixelFormat::RGB24 && roughness_channel < 3)); // Roughness with PixelFormat::RGB24 must use channels one, two or three for roughness.
        // Sanitize roughness channel index based on pixel format. Fx for Alpha8 the channel index is 3, but since only one channel contains information, the channel index must be reduced to 0 for direct access.
        roughness_channel = min(roughness_channel, roughness_pixel_size - 1);

        Image tint_roughness = Images::create2D(tint.get_name() + "_" + roughness.get_name(), PixelFormat::RGBA32, tint.get_gamma(), size, mipmap_count);

        const unsigned char* tint_pixels = (const unsigned char*)tint.get_pixels();
        const unsigned char* roughness_pixels = (unsigned char*)roughness.get_pixels() + roughness_channel;
        RGBA32* tint_roughness_pixels = tint_roughness.get_pixels<RGBA32>();

        #pragma omp parallel for schedule(dynamic, 16)
        for (int c = 0; c < chunk_count; ++c) {

            int pixel_begin = c * chunk_size;
            int pixel_end = min(pixel_begin + chunk_size, pixel_count);

            // Fill tint channels
            for (int p = pixel_begin; p < pixel_end; ++p) {
                tint_roughness_pixels[p].r = tint_pixels[p * tint_pixel_size];
                tint_roughness_pixels[p].g = tint_pixels[p * tint_pixel_size + 1];
                tint_roughness_pixels[p].b = tint_pixels[p * tint_pixel_size + 2];
            }

            // Fill roughness channels
            if (roughness_format == PixelFormat::Roughness8 || roughness.get_gamma() == 1.0f)
                // Roughnes is stored in alpha and should not be gamma corrected.
                for (int p = pixel_begin; p < pixel_end; ++p)
                    tint_roughness_pixels[p].a = roughness_pixels[p * roughness_pixel_size];
            else
                // Roughness is stored as a color and should be degammaed.
                for (int p = pixel_begin; p < pixel_end; ++p) {
                    float nonlinear_roughness = roughness_pixels[p * roughness_pixel_size] / 255.0f;
                    float linear_roughness = powf(nonlinear_roughness, roughness.get_gamma());
                    tint_roughness_pixels[p].a = unsigned char(linear_roughness * 255 + 0.5f);
                }
        }

        return tint_roughness;

    } else {
        // Fallback path
        Image tint_roughness = Images::create2D(tint.get_name() + "_" + roughness.get_name(), PixelFormat::RGBA32, 2.2f, size, mipmap_count);

        #pragma omp parallel for schedule(dynamic, 16)
        for (int c = 0; c < chunk_count; ++c) {

            int pixel_begin = c * chunk_size;
            int pixel_end = min(pixel_begin + chunk_size, pixel_count);

            // Fill tint and roughness channels.
            for (int p = pixel_begin; p < pixel_end; ++p) {
                RGB t = tint.get_pixel(p).rgb();
                float r = roughness.get_pixel(p)[roughness_channel];
                tint_roughness.set_pixel(RGBA(t, r), p);
            }
        }

        return tint_roughness;
    }
}

} // NS ImageUtils

} // NS Assets
} // NS Bifrost
