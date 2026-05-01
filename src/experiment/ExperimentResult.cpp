#include "ExperimentResult.hpp"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace qi::experiment {

using qi::geometry::QuadricParams;
using qi::geometry::Quat;
using qi::geometry::Transform;
using qi::geometry::Vec3;

QString quadricParamsToJson(const QuadricParams& params) {
    QJsonObject obj;
    obj["a"] = params.a;
    obj["b"] = params.b;
    obj["c"] = params.c;
    obj["p"] = params.p;
    return QString::fromUtf8(QJsonDocument(obj).toJson(QJsonDocument::Compact));
}

QuadricParams quadricParamsFromJson(const QString& json) {
    const QJsonObject obj =
        QJsonDocument::fromJson(json.toUtf8()).object();
    QuadricParams p;
    p.a = obj.value("a").toDouble(1.0);
    p.b = obj.value("b").toDouble(1.0);
    p.c = obj.value("c").toDouble(1.0);
    p.p = obj.value("p").toDouble(1.0);
    return p;
}

QString transformToJson(const Transform& t) {
    QJsonArray translation;
    translation.append(t.translation().x());
    translation.append(t.translation().y());
    translation.append(t.translation().z());

    // Rotation stored as (x, y, z, w) — same layout as Eigen's coeffs() and
    // most JSON quaternion conventions.
    QJsonArray rotation;
    rotation.append(t.rotation().x());
    rotation.append(t.rotation().y());
    rotation.append(t.rotation().z());
    rotation.append(t.rotation().w());

    QJsonObject obj;
    obj["translation"] = translation;
    obj["rotation"] = rotation;
    return QString::fromUtf8(QJsonDocument(obj).toJson(QJsonDocument::Compact));
}

Transform transformFromJson(const QString& json) {
    const QJsonObject obj =
        QJsonDocument::fromJson(json.toUtf8()).object();
    const QJsonArray tr = obj.value("translation").toArray();
    const QJsonArray rt = obj.value("rotation").toArray();

    const Vec3 translation(tr.at(0).toDouble(0.0),
                           tr.at(1).toDouble(0.0),
                           tr.at(2).toDouble(0.0));
    // Eigen's Quaterniond constructor takes (w, x, y, z).
    const Quat rotation(rt.at(3).toDouble(1.0),
                        rt.at(0).toDouble(0.0),
                        rt.at(1).toDouble(0.0),
                        rt.at(2).toDouble(0.0));
    return Transform(translation, rotation);
}

QString triangulationParamsToJson(const SurfaceRecord& s) {
    QJsonObject obj;
    if (s.triangulationMethod == "marching_cubes") {
        obj["resolution"] = s.mcResolution;
    } else if (s.triangulationMethod == "parametric") {
        obj["uSteps"] = s.uSteps;
        obj["vSteps"] = s.vSteps;
    }
    return QString::fromUtf8(QJsonDocument(obj).toJson(QJsonDocument::Compact));
}

void triangulationParamsFromJson(const QString& json, SurfaceRecord& s) {
    const QJsonObject obj =
        QJsonDocument::fromJson(json.toUtf8()).object();
    if (obj.contains("resolution")) {
        s.mcResolution = obj.value("resolution").toInt(s.mcResolution);
    }
    if (obj.contains("uSteps")) {
        s.uSteps = obj.value("uSteps").toInt(s.uSteps);
    }
    if (obj.contains("vSteps")) {
        s.vSteps = obj.value("vSteps").toInt(s.vSteps);
    }
}

}  // namespace qi::experiment
