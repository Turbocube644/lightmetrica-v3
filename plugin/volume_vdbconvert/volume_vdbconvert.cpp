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

class Volume_VdbConvertScalar : public Volume {
private:
    std::string getAbsolutePath(std::string filename)
    {
        const int MAX_LENTH = 4096;
        char resolved_path[MAX_LENTH];
        _fullpath(resolved_path, filename.c_str(), MAX_LENTH);
        return std::string(resolved_path);
    }

    bool endsWith(const std::string s, const std::string end)
    {
        size_t len = end.size();
        if (len > s.size()) return false;

        std::string substr = s.substr(s.size() - len, len);
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
        size_t idx = path.find("/", 0) + 1;
        return upath.substr(idx, upath.length() - idx);
    }

public:
    Volume_VdbConvertScalar() {
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
    }

private:
    bool convert(const std::string& path, const Json& prop) {
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
        Float scale = json::value<Float>(prop, "scale", 1_f);
        Float step_size = json::value<Float>(prop, "step_size", .1_f);

        // Bound
        const auto b = vdbloaderGetBound(context);
        Bound bound;
        bound.min = Vec3(b.min.x, b.min.y, b.min.z);
        bound.max = Vec3(b.max.x, b.max.y, b.max.z);

        // Maximum density
        Float max_scalar = vbdloaderGetMaxScalar(context) * scale;

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

        std::ofstream meta_file_stream(new_path_meta, std::ios::out);
        meta_file_stream << json.dump(4);
        meta_file_stream.close();

        // create large enough buffer
        // auto volume_size = sizeof(Float) * x_steps * y_steps * z_steps;
        // auto* volume = static_cast<Float*>(malloc(volume_size));

        std::ofstream converted_file_stream(new_path, std::ios::out | std::ios::binary);
        for(int z = 0; z < z_steps; z++) {
            for (int y = 0; y < y_steps; y++) {
                for (int x = 0; x < x_steps; x++) {
                    // volume[z * x_steps * y_steps + y * x_steps + x] = 
                    Float value = vbdloaderEvalScalar(context, VDBLoaderFloat3{bound.min.x + x * step_size, bound.min.y + y * step_size, bound.min.z + z * step_size});
                    converted_file_stream.write(reinterpret_cast<char*>(&value), sizeof(Float));
                }
            }
        }
        converted_file_stream.close();

        vdbloaderReleaseContext(context);

        return true;
    }

public:
    virtual void construct(const Json& prop) override {
        // Load VDB file
        const auto path = json::value<std::string>(prop, "path");
        if (endsWith(path, ".vdb")) {
            LM_INFO("Attempting old vdb load");
            convert(path, prop);
        }

        // TODO
        LM_INFO("Add some logic here");
    }

    virtual Bound bound() const override {
        return Bound();
    }

    virtual Float max_scalar() const override {
        return 0.0;
    }

    virtual bool has_scalar() const override {
        return true;
    }

    virtual Float eval_scalar(Vec3 p) const override {
        return 1.0;
        // const auto d = vbdloaderEvalScalar(context_, VDBLoaderFloat3{ p.x, p.y, p.z });
        // return d * scale_;
    }

    virtual bool has_color() const override {
        return false;
    }

    virtual void march(Ray ray, Float tmin, Float tmax, Float marchStep, const RaymarchFunc& raymarchFunc) const override {
        exception::ScopedDisableFPEx guard_;
        const void* f = reinterpret_cast<const void*>(&raymarchFunc);
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
