#include "ConfigJson.hpp"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

#include "ExperimentResult.hpp"

namespace qi::experiment {

using qi::geometry::BoundingBox;
using qi::geometry::Vec3;

namespace {

QJsonArray vec3ToArray(const Vec3& v) {
    QJsonArray a;
    a.append(v.x());
    a.append(v.y());
    a.append(v.z());
    return a;
}

Vec3 arrayToVec3(const QJsonArray& a, const Vec3& fallback) {
    if (a.size() < 3) return fallback;
    return Vec3(a.at(0).toDouble(fallback.x()),
                a.at(1).toDouble(fallback.y()),
                a.at(2).toDouble(fallback.z()));
}

QJsonObject bboxToJson(const BoundingBox& b) {
    QJsonObject obj;
    obj["min"] = vec3ToArray(b.min());
    obj["max"] = vec3ToArray(b.max());
    return obj;
}

BoundingBox bboxFromJson(const QJsonObject& obj) {
    const Vec3 mn = arrayToVec3(obj.value("min").toArray(), Vec3(-1, -1, -1));
    const Vec3 mx = arrayToVec3(obj.value("max").toArray(), Vec3(1, 1, 1));
    return BoundingBox(mn, mx);
}

QJsonObject surfaceToJson(const SurfaceConfig& sc) {
    QJsonObject obj;
    obj["type"] = sc.type;
    // Reuse the existing primitives so the on-disk format matches the values
    // stored in DB (params_json / transform_json columns).
    obj["params"] = QJsonDocument::fromJson(quadricParamsToJson(sc.params).toUtf8()).object();
    obj["transform"] = QJsonDocument::fromJson(transformToJson(sc.transform).toUtf8()).object();
    obj["triangulationMethod"] = sc.triangulationMethod;
    obj["uSteps"] = sc.uSteps;
    obj["vSteps"] = sc.vSteps;
    obj["mcResolution"] = sc.mcResolution;
    return obj;
}

SurfaceConfig surfaceFromJson(const QJsonObject& obj) {
    SurfaceConfig sc;
    sc.type = obj.value("type").toString("ellipsoid");
    sc.params = quadricParamsFromJson(QString::fromUtf8(
        QJsonDocument(obj.value("params").toObject()).toJson(QJsonDocument::Compact)));
    sc.transform = transformFromJson(QString::fromUtf8(
        QJsonDocument(obj.value("transform").toObject()).toJson(QJsonDocument::Compact)));
    sc.triangulationMethod = obj.value("triangulationMethod").toString("parametric");
    sc.uSteps = obj.value("uSteps").toInt(40);
    sc.vSteps = obj.value("vSteps").toInt(40);
    sc.mcResolution = obj.value("mcResolution").toInt(32);
    return sc;
}

}  // namespace

QString configToJson(const ExperimentConfig& cfg) {
    QJsonObject obj;
    obj["bbox"] = bboxToJson(cfg.bbox);
    obj["intersectionMethod"] = cfg.intersectionMethod;
    obj["notes"] = cfg.notes;

    QJsonArray surfaces;
    for (const auto& sc : cfg.surfaces) {
        surfaces.append(surfaceToJson(sc));
    }
    obj["surfaces"] = surfaces;

    return QString::fromUtf8(QJsonDocument(obj).toJson(QJsonDocument::Indented));
}

ExperimentConfig configFromJson(const QString& json) {
    ExperimentConfig cfg;
    const QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
    if (!doc.isObject()) return cfg;

    const QJsonObject obj = doc.object();
    cfg.bbox = bboxFromJson(obj.value("bbox").toObject());
    cfg.intersectionMethod = obj.value("intersectionMethod").toString("bvh");
    cfg.notes = obj.value("notes").toString();

    const QJsonArray surfaces = obj.value("surfaces").toArray();
    cfg.surfaces.reserve(static_cast<std::size_t>(surfaces.size()));
    for (const auto& v : surfaces) {
        cfg.surfaces.push_back(surfaceFromJson(v.toObject()));
    }
    return cfg;
}

bool saveConfigToFile(const ExperimentConfig& cfg, const QString& path) {
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        return false;
    }
    const QByteArray bytes = configToJson(cfg).toUtf8();
    return f.write(bytes) == bytes.size();
}

std::optional<ExperimentConfig> loadConfigFromFile(const QString& path) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return std::nullopt;
    }
    return configFromJson(QString::fromUtf8(f.readAll()));
}

}  // namespace qi::experiment
