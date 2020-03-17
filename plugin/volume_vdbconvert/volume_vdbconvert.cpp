/*
    Lightmetrica - Copyright (c) 2020 Felix Heim
    Distributed under MIT license. See LICENSE file for details.
*/

#include <lm/volume.h>
#include <lm/core.h>
#include <vdbloader.h>
#include <fstream>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

#define VDB_ENDING ".vdb"
#define NEW_ENDING ".cvdb"
#define META_ENDING ".json"

using Vec3i = glm::tvec3<int>;

class Volume_VdbConvertScalar : public Volume {
private:
    Bound bound_;
    Float max_scalar_{};
    float* volume_;
    Vec3i dimension_{};

    std::string getAbsolutePath(std::string filename)
    {
        const int MAX_LENTH = 4096;
        char resolved_path[MAX_LENTH];
        _fullpath(resolved_path, filename.c_str(), MAX_LENTH);
        return std::string(resolved_path);
    }

    bool endsWith(const std::string s, const std::string end)
    {
        const size_t len = end.size();
        if (len > s.size()) return false;

        const std::string substr = s.substr(s.size() - len, len);
        return end == substr;
    }

    std::string unixifyPath(std::string path)
    {
        size_t index = 0;
        while (true)
        {
            index = path.find("\\", index);
            if (index == std::string::npos) break;

            path.replace(index, 1, "/");
            index += 1;
        }

        return path;
    }

    std::string getFileName(const std::string path)
    {
        const std::string upath = unixifyPath(path);
        const size_t idx = path.find("/", 0) + 1;
        return upath.substr(idx, upath.length() - idx);
    }

public:
    Volume_VdbConvertScalar() : volume_(nullptr) {
    }

private:
    bool convert(const std::string& path, const Json& prop) {
        vdbloaderSetErrorFunc(nullptr, [](void*, int errorCode, const char* message) {
            const auto errStr = [&]() -> std::string {
                if (errorCode == VDBLOADER_ERROR_INVALID_CONTEXT)
                    return "INVALID_CONTEXT";
                else if (errorCode == VDBLOADER_ERROR_INVALID_ARGUMENT)
                    return "INVALID_ARGUMENT";
                else if (errorCode == VDBLOADER_ERROR_UNKNOWN)
                    return "UNKNOWN";
                LM_UNREACHABLE_RETURN();
            }();
            LM_ERROR("vdbloader error: {} [type='{}']", message, errStr);
        });

        VDBLoaderContext context = vdbloaderCreateContext();
        LM_INFO("Opening OpenVDB file [path='{}']", path);
        if (!vdbloaderLoadVDBFile(context, path.c_str())) {
            LM_THROW_EXCEPTION(Error::IOError, "Failed to load OpenVDB file [path='{}']", path);
        }

        const std::string new_path_base = path.substr(0, path.find(VDB_ENDING));
        std::string new_path = new_path_base + NEW_ENDING;
        std::string new_path_meta = new_path_base + META_ENDING;
        LM_INFO("Loaded OpnVDB. Now converting and saving to new format [path='{}'] with meta file [path='{}']", new_path, new_path_meta);

        // Density scale, remove?
        const Float scale = json::value<Float>(prop, "scale", 1_f);
        const Float step_size = json::value<Float>(prop, "step_size", .1_f);

        // Bound
        const auto b = vdbloaderGetBound(context);
        Bound bound;
        bound.min = Vec3(b.min.x, b.min.y, b.min.z);
        bound.max = Vec3(b.max.x, b.max.y, b.max.z);

        // Maximum density
        Float max_scalar = vbdloaderGetMaxScalar(context) * scale;
        LM_INFO("Max Scalar: {}", max_scalar);

        LM_INFO("Bound of Volume: [{}, {}, {}] to [{}, {}, {}].", 
            bound.min.x, 
            bound.min.y, 
            bound.min.z,
            bound.max.x,
            bound.max.y,
            bound.max.z);

        LM_INFO("Chosen Step Size: {}", step_size);

        const Vec3 range = bound.max - bound.min;
        Float x_remainder = fmod(range.x, step_size);
        Float y_remainder = fmod(range.y, step_size);
        Float z_remainder = fmod(range.z, step_size);
        if (x_remainder < Eps) {
            x_remainder = step_size;
        }
        if (y_remainder < Eps) {
            y_remainder = step_size;
        }
        if (z_remainder < Eps) {
            z_remainder = step_size;
        }
        Vec3 correction = (Vec3(step_size) - Vec3(x_remainder, y_remainder, z_remainder)) / 2_f;
        bound.min -= correction;
        bound.max += correction;
        const Vec3 step_counts = (bound.max - bound.min) / step_size + Vec3(1);

        int x_steps = int(std::round(step_counts.x));
        int y_steps = int(std::round(step_counts.y)); 
        int z_steps = int(std::round(step_counts.z));

        LM_INFO("Extended Bound of Volume adapted to step_size: [{}, {}, {}] to [{}, {}, {}].",
            bound.min.x,
            bound.min.y,
            bound.min.z,
            bound.max.x,
            bound.max.y,
            bound.max.z);

        LM_INFO("Step Counts: {}, {}, {}", x_steps, y_steps, z_steps);

        Json json;
        Json jsonBound;
        Json jsonBoundMin;
        jsonBoundMin.emplace("x", bound.min.x);
        jsonBoundMin.emplace("y", bound.min.y);
        jsonBoundMin.emplace("z", bound.min.z);
        jsonBound.emplace("min", jsonBoundMin);
        Json jsonBoundMax;
        jsonBoundMax.emplace("x", bound.max.x);
        jsonBoundMax.emplace("y", bound.max.y);
        jsonBoundMax.emplace("z", bound.max.z);
        jsonBound.emplace("max", jsonBoundMax);
        json.emplace("bound", jsonBound);
        Json jsonDimension;
        jsonDimension.emplace("x", x_steps);
        jsonDimension.emplace("y", y_steps);
        jsonDimension.emplace("z", z_steps);
        json.emplace("dimension", jsonDimension);
        json.emplace("step_size", step_size);
        json.emplace("max_scalar", max_scalar);

        std::ofstream meta_file_stream(new_path_meta, std::ios::out);
        meta_file_stream << json.dump(4);
        meta_file_stream.close();

        std::ofstream converted_file_stream(new_path, std::ios::out | std::ios::binary);
        for(int z = 0; z < z_steps; z++) {
            for (int y = 0; y < y_steps; y++) {
                for (int x = 0; x < x_steps; x++) {
                    float value = float(vbdloaderEvalScalar(context, VDBLoaderFloat3{bound.min.x + x * step_size, bound.min.y + y * step_size, bound.min.z + z * step_size}));
                    converted_file_stream.write(reinterpret_cast<char*>(&value), sizeof(float));
                }
            }
        }
        converted_file_stream.close();

        LM_INFO("Point at (0, 0, 0): {}, {}", float(vbdloaderEvalScalar(context, VDBLoaderFloat3{0, 0, 0})), vbdloaderEvalScalar(context, VDBLoaderFloat3{ 0, 0, 0 }));

        vdbloaderReleaseContext(context);

        return true;
    }

    float eval_scalar(int x, int y, int z) const {
        // first check if in bound. If out of bound --> Zero Density
        if (x < 0 || y < 0 || z < 0
            || x >= dimension_.x || y >= dimension_.y || z >= dimension_.z) {
            return 0.0f;
        }
        // otherwise read from float array with data
        return volume_[z * dimension_.x * dimension_.y + y * dimension_.x + x];
    }

    float lerp(int x1, int y1, int z1, int x2, int y2, int z2, float t) const {
        return lerp(eval_scalar(x1, y1, z1), eval_scalar(x2, y2, z2), t);
    }

    float lerp(float a, float b, float t) const {
        return a + t * (b - a);
    }

public:
    virtual void construct(const Json& prop) override {
        // Load VDB file
        std::string path_base;
        const auto path = json::value<std::string>(prop, "path");
        if (endsWith(path, ".vdb")) {
            LM_INFO("Attempting old vdb load");
            convert(path, prop);
            path_base = path.substr(0, path.find(VDB_ENDING));
        } else {
            path_base = path.substr(0, path.find(NEW_ENDING));
        }
        std::string path_converted = path_base + NEW_ENDING;
        std::string path_meta = path_base + META_ENDING;

        std::ifstream meta_stream(path_meta);
        Json meta = Json::parse(meta_stream);
        const auto step_size = json::value<Float>(meta, "step_size");
        const auto dimension = json::value<Json>(meta, "dimension");
        dimension_ = Vec3i(json::value<int>(dimension, "x"), json::value<int>(dimension, "y"), json::value<int>(dimension, "z"));
        Json bound = json::value<Json>(meta, "bound");
        Json boundMin = json::value<Json>(bound, "min");
        Json boundMax = json::value<Json>(bound, "max");
        bound_.min = Vec3(json::value<Float>(boundMin, "x"), json::value<Float>(boundMin, "y"), json::value<Float>(boundMin, "z"));
        bound_.max = Vec3(json::value<Float>(boundMax, "x"), json::value<Float>(boundMax, "y"), json::value<Float>(boundMax, "z"));
        max_scalar_ = json::value<Float>(meta, "max_scalar");

        LM_INFO("Loaded Converted Volume File");
        LM_INFO("Extended Bound of Volume adapted to step_size: [{}, {}, {}] to [{}, {}, {}].",
            bound_.min.x,
            bound_.min.y,
            bound_.min.z,
            bound_.max.x,
            bound_.max.y,
            bound_.max.z);
        LM_INFO("Step Counts: {}, {}, {}", dimension_.x, dimension_.y, dimension_.z);
        LM_INFO("Max Scalar: {}", max_scalar_);

        // create large enough buffer
        auto volume_size = sizeof(float) * dimension_.x * dimension_.y * dimension_.z;
        volume_ = static_cast<float*>(malloc(volume_size));
        std::ifstream vdb_stream(path_meta, std::ios::binary);
        vdb_stream.read(reinterpret_cast<char*>(volume_), volume_size);

        LM_INFO("Point at (0, 0, 0): {}", eval_scalar(Vec3(0)));
    }

    virtual Bound bound() const override {
        return bound_;
    }

    virtual Float max_scalar() const override {
        return max_scalar_;
    }

    virtual bool has_scalar() const override {
        return true;
    }

    virtual Float eval_scalar(Vec3 p) const override {
        // Compute uv in bound
        Vec3 p_boundSpace = (p - bound_.min) / (bound_.max - bound_.min);
        // scale to address in volume array
        p_boundSpace *= dimension_;

        // aligned cube around our position p
        const Vec3i lowerLeft = { int(std::floor(p_boundSpace.x)), int(std::floor(p_boundSpace.y)), int(std::floor(p_boundSpace.z)) };
        const Vec3i upperRight = lowerLeft + Vec3i(1);
        // factor for trilinear interpolation
        const Vec3 t = p_boundSpace - Vec3(lowerLeft);
        
        // first interpolate on x-axis, then y-axis, z-axis
        const float x00 = lerp(lowerLeft.x, lowerLeft.y, lowerLeft.z, upperRight.x, lowerLeft.y, lowerLeft.z, float(t.x));
        const float x01 = lerp(lowerLeft.x, lowerLeft.y, upperRight.z, upperRight.x, lowerLeft.y, upperRight.z, float(t.x));
        const float x10 = lerp(lowerLeft.x, upperRight.y, lowerLeft.z, upperRight.x, upperRight.y, lowerLeft.z, float(t.x));
        const float x11 = lerp(lowerLeft.x, upperRight.y, upperRight.z, upperRight.x, upperRight.y, upperRight.z, float(t.x));
        // now y-axis
        const float y0 = lerp(x00, x10, float(t.y));
        const float y1 = lerp(x01, x11, float(t.y));
        // final interpolation
        float z = lerp(y0, y1, float(t.z));

        return Float(z);
        // const auto d = vbdloaderEvalScalar(context_, VDBLoaderFloat3{ p.x, p.y, p.z });
        // return d * scale_;
        // 
        // volume[z * x_steps * y_steps + y * x_steps + x] = 
    }

    virtual bool has_color() const override {
        return false;
    }

    virtual void march(Ray ray, Float tmin, Float tmax, Float marchStep, const RaymarchFunc& raymarchFunc) const override {
        LM_ERROR("Not impleneted!");
        // exception::ScopedDisableFPEx guard_;
        // const void* f = reinterpret_cast<const void*>(&raymarchFunc);
        /*vdbloaderMarchVolume(
            context_,
            VDBLoaderFloat3{ ray.o.x, ray.o.y, ray.o.z },
            VDBLoaderFloat3{ ray.d.x, ray.d.y, ray.d.z },
            tmin, tmax, marchStep, const_cast<void*>(f),
            [](void* user, double t) -> bool {
                const void* f = const_cast<const void*>(user);
                const RaymarchFunc& raymarchFunc = *reinterpret_cast<const RaymarchFunc*>(f);
                return raymarchFunc(t);
            });*/
    }
};

LM_COMP_REG_IMPL(Volume_VdbConvertScalar, "volume::vdb_convert");

#undef META_ENDING
#undef NEW_ENDING
#undef VDB_ENDING

LM_NAMESPACE_END(LM_NAMESPACE)
