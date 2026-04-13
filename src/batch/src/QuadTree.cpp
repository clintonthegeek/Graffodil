#include <graffodil/QuadTree.h>
#include <cmath>

namespace Graffodil {

void QuadTree::clear()
{
    m_nodes.clear();
    m_masses.clear();
    m_root = -1;
}

void QuadTree::build(const QVector<QPointF> &positions, const QVector<qreal> &masses)
{
    clear();
    if (positions.isEmpty())
        return;

    m_masses.resize(positions.size());
    for (int i = 0; i < positions.size(); ++i)
        m_masses[i] = masses.isEmpty() ? 1.0 : masses.value(i, 1.0);

    // Compute bounds from positions
    qreal minX = positions[0].x(), maxX = minX;
    qreal minY = positions[0].y(), maxY = minY;
    for (int i = 1; i < positions.size(); ++i) {
        minX = qMin(minX, positions[i].x());
        maxX = qMax(maxX, positions[i].x());
        minY = qMin(minY, positions[i].y());
        maxY = qMax(maxY, positions[i].y());
    }
    // Add small padding to avoid zero-size bounds
    QRectF bounds(minX - 1, minY - 1, (maxX - minX) + 2, (maxY - minY) + 2);

    m_nodes.reserve(5 * positions.size());
    QuadNode root;
    root.bounds = bounds;
    m_nodes.append(root);
    m_root = 0;

    for (int i = 0; i < positions.size(); ++i)
        insert(m_root, i, positions);
}

void QuadTree::subdivide(int quadNodeIdx)
{
    QuadNode &qn = m_nodes[quadNodeIdx];
    const QRectF &b = qn.bounds;
    double hw = b.width() / 2.0;
    double hh = b.height() / 2.0;

    // NW, NE, SW, SE
    QRectF quads[4] = {
        QRectF(b.left(),      b.top(),       hw, hh),
        QRectF(b.left() + hw, b.top(),       hw, hh),
        QRectF(b.left(),      b.top() + hh,  hw, hh),
        QRectF(b.left() + hw, b.top() + hh,  hw, hh),
    };

    for (int q = 0; q < 4; ++q) {
        QuadNode child;
        child.bounds = quads[q];
        m_nodes.append(child);
        // Re-fetch reference after potential reallocation
        m_nodes[quadNodeIdx].children[q] = m_nodes.size() - 1;
    }
}

void QuadTree::insert(int quadNodeIdx, int nodeIdx, const QVector<QPointF> &positions, int depth)
{
    // Guard against excessive depth (e.g. coincident points)
    if (depth >= MAX_DEPTH)
        return;

    QuadNode &qn = m_nodes[quadNodeIdx];

    // Update center of mass and total mass
    const QPointF &pos = positions[nodeIdx];
    double nodeMass = m_masses[nodeIdx];

    if (qn.isEmpty()) {
        // Empty leaf — store the point here
        qn.nodeIndex = nodeIdx;
        qn.centerOfMass = pos;
        qn.totalMass = nodeMass;
        return;
    }

    // Update aggregate mass
    double newTotal = qn.totalMass + nodeMass;
    qn.centerOfMass = QPointF(
        (qn.centerOfMass.x() * qn.totalMass + pos.x() * nodeMass) / newTotal,
        (qn.centerOfMass.y() * qn.totalMass + pos.y() * nodeMass) / newTotal
    );
    qn.totalMass = newTotal;

    if (qn.isLeaf()) {
        // Internal point — need to subdivide and re-insert existing point
        int existingIdx = qn.nodeIndex;
        qn.nodeIndex = -1;
        subdivide(quadNodeIdx);

        // Re-insert existing point into appropriate child
        const QPointF &existingPos = positions[existingIdx];
        const QRectF &b = m_nodes[quadNodeIdx].bounds;
        int q = (existingPos.x() >= b.left() + b.width() / 2.0 ? 1 : 0)
              + (existingPos.y() >= b.top() + b.height() / 2.0 ? 2 : 0);
        insert(m_nodes[quadNodeIdx].children[q], existingIdx, positions, depth + 1);
    }

    // Insert new point into appropriate child
    const QRectF &b = m_nodes[quadNodeIdx].bounds;
    int q = (pos.x() >= b.left() + b.width() / 2.0 ? 1 : 0)
          + (pos.y() >= b.top() + b.height() / 2.0 ? 2 : 0);
    insert(m_nodes[quadNodeIdx].children[q], nodeIdx, positions, depth + 1);
}

QPointF QuadTree::computeRepulsion(const QPointF &pos, qreal repelForce,
                                    qreal nodeMass, qreal theta) const
{
    if (m_root < 0)
        return QPointF(0, 0);

    QPointF force(0, 0);

    // Iterative traversal using a stack
    QVector<int> stack;
    stack.reserve(64);
    stack.append(m_root);

    while (!stack.isEmpty()) {
        int idx = stack.takeLast();
        if (idx < 0 || idx >= m_nodes.size())
            continue;

        const QuadNode &qn = m_nodes[idx];
        if (qn.isEmpty())
            continue;

        double dx = qn.centerOfMass.x() - pos.x();
        double dy = qn.centerOfMass.y() - pos.y();
        double distSq = dx * dx + dy * dy;

        if (distSq < 1e-10)
            continue; // skip self or coincident

        double size = qn.bounds.width(); // use width as region size

        // Barnes-Hut criterion: if leaf or region is far enough, treat as single body
        if (qn.isLeaf() || (size * size / distSq < theta * theta)) {
            double dist = std::sqrt(distSq);
            double magnitude = repelForce * nodeMass * qn.totalMass / distSq;
            force += QPointF(-dx / dist * magnitude, -dy / dist * magnitude);
        } else {
            // Recurse into children
            for (int q = 0; q < 4; ++q) {
                if (qn.children[q] >= 0)
                    stack.append(qn.children[q]);
            }
        }
    }

    return force;
}

} // namespace Graffodil
