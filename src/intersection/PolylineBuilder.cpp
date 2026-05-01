#include "PolylineBuilder.hpp"

#include <cstddef>
#include <optional>
#include <utility>
#include <vector>

#include "Vec3.hpp"

namespace qi::intersection {

using qi::geometry::Vec3;
using qi::mesh::Polyline;
using qi::mesh::Segment;

namespace {

using PointIdx = std::size_t;
using EdgeIdx = std::size_t;

struct Graph {
    std::vector<Vec3> points;
    std::vector<std::pair<PointIdx, PointIdx>> edges;
    std::vector<std::vector<EdgeIdx>> adjacency;
};

PointIdx findOrAdd(std::vector<Vec3>& points, const Vec3& p, double epsilon) {
    const double eps2 = epsilon * epsilon;
    for (PointIdx i = 0; i < points.size(); ++i) {
        if ((points[i] - p).squaredNorm() < eps2) return i;
    }
    points.push_back(p);
    return points.size() - 1;
}

Graph buildGraph(const std::vector<Segment>& segments, double epsilon) {
    Graph g;
    g.edges.reserve(segments.size());
    for (const auto& s : segments) {
        const PointIdx a = findOrAdd(g.points, s.a, epsilon);
        const PointIdx b = findOrAdd(g.points, s.b, epsilon);
        if (a == b) continue;  // degenerate segment, skip
        g.edges.emplace_back(a, b);
    }
    g.adjacency.assign(g.points.size(), {});
    for (EdgeIdx e = 0; e < g.edges.size(); ++e) {
        g.adjacency[g.edges[e].first].push_back(e);
        g.adjacency[g.edges[e].second].push_back(e);
    }
    return g;
}

PointIdx otherEnd(const std::pair<PointIdx, PointIdx>& edge, PointIdx fromPoint) {
    return (edge.first == fromPoint) ? edge.second : edge.first;
}

std::optional<EdgeIdx> findUnusedNeighbor(const Graph& g,
                                          const std::vector<bool>& usedEdge,
                                          PointIdx pointIdx,
                                          EdgeIdx excludeEdge) {
    for (EdgeIdx e : g.adjacency[pointIdx]) {
        if (e == excludeEdge) continue;
        if (usedEdge[e]) continue;
        return e;
    }
    return std::nullopt;
}

}  // namespace

std::vector<Polyline> buildPolylines(const std::vector<Segment>& segments, double epsilon) {
    Graph g = buildGraph(segments, epsilon);

    std::vector<Polyline> result;
    if (g.edges.empty()) return result;

    std::vector<bool> usedEdge(g.edges.size(), false);

    for (EdgeIdx startEdge = 0; startEdge < g.edges.size(); ++startEdge) {
        if (usedEdge[startEdge]) continue;
        usedEdge[startEdge] = true;

        const PointIdx startPoint = g.edges[startEdge].first;
        std::vector<PointIdx> chain = {startPoint, g.edges[startEdge].second};
        PointIdx curPoint = chain.back();
        EdgeIdx lastEdge = startEdge;
        bool closed = false;

        while (true) {
            const auto next = findUnusedNeighbor(g, usedEdge, curPoint, lastEdge);
            if (!next.has_value()) break;
            usedEdge[*next] = true;
            curPoint = otherEnd(g.edges[*next], curPoint);
            chain.push_back(curPoint);
            lastEdge = *next;
            if (curPoint == startPoint) {
                closed = true;
                break;
            }
        }

        if (!closed) {
            // Walk the other direction from startPoint, prepending points.
            PointIdx curBack = startPoint;
            EdgeIdx lastBackEdge = startEdge;
            while (true) {
                const auto next = findUnusedNeighbor(g, usedEdge, curBack, lastBackEdge);
                if (!next.has_value()) break;
                usedEdge[*next] = true;
                curBack = otherEnd(g.edges[*next], curBack);
                chain.insert(chain.begin(), curBack);
                lastBackEdge = *next;
            }
        }

        Polyline poly;
        poly.closed = closed;
        poly.points.reserve(chain.size());
        for (PointIdx idx : chain) {
            poly.points.push_back(g.points[idx]);
        }
        result.push_back(std::move(poly));
    }

    return result;
}

}  // namespace qi::intersection
