#pragma once

#include <graffodil/IGraphNode.h>
#include <graffodil/Anchors.h>
#include <QGraphicsRectItem>
#include <QGraphicsEllipseItem>
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QVector>
#include <cmath>

/// Standard compass node — rectangle with Top/Right/Bottom/Left anchors.
/// Matches the default behavior expected by most graph-drawing use cases.
class CompassNode : public QGraphicsRectItem, public Graffodil::IGraphNode {
public:
    explicit CompassNode(const QString &id, const QPointF &pos = {})
        : QGraphicsRectItem(0, 0, 100, 60), m_id(id)
    {
        setPos(pos);
        setFlag(ItemIsSelectable);
        setBrush(QColor(70, 130, 180));
        setPen(QPen(QColor(40, 80, 120), 1.5));
        setCursor(Qt::OpenHandCursor);
    }

    QString nodeId() const override { return m_id; }
    QList<Graffodil::Anchor> anchors() const override {
        return Graffodil::compassAnchors(mapToScene(rect()).boundingRect());
    }
    QRectF nodeBoundingRect() const override { return mapToScene(rect()).boundingRect(); }
    QGraphicsItem *graphicsItem() override { return this; }

protected:
    void paint(QPainter *p, const QStyleOptionGraphicsItem *opt, QWidget *w) override {
        QGraphicsRectItem::paint(p, opt, w);
        p->save();
        p->setPen(Qt::white);
        QFont f = p->font(); f.setPointSize(9); f.setBold(true); p->setFont(f);
        p->drawText(rect(), Qt::AlignCenter, m_id);
        p->restore();
    }

private:
    QString m_id;
};

/// Dependency-style node — rectangle with only Left/Right anchors.
/// Flatter, color-coded to signal the simplified connection model.
class DependencyNode : public QGraphicsRectItem, public Graffodil::IGraphNode {
public:
    explicit DependencyNode(const QString &id, const QPointF &pos = {})
        : QGraphicsRectItem(0, 0, 120, 40), m_id(id)
    {
        setPos(pos);
        setFlag(ItemIsSelectable);
        setBrush(QColor(180, 140, 70));
        setPen(QPen(QColor(120, 90, 40), 1.5));
        setCursor(Qt::OpenHandCursor);
    }

    QString nodeId() const override { return m_id; }
    QList<Graffodil::Anchor> anchors() const override {
        const QRectF r = mapToScene(rect()).boundingRect();
        QList<Graffodil::Anchor> list;
        Graffodil::Anchor l;
        l.id = Graffodil::AnchorIds::Left;
        l.scenePos = {r.left(), r.center().y()};
        l.outwardDirection = {-1, 0};
        list.append(l);
        Graffodil::Anchor ri;
        ri.id = Graffodil::AnchorIds::Right;
        ri.scenePos = {r.right(), r.center().y()};
        ri.outwardDirection = {1, 0};
        list.append(ri);
        return list;
    }
    QRectF nodeBoundingRect() const override { return mapToScene(rect()).boundingRect(); }
    QGraphicsItem *graphicsItem() override { return this; }

protected:
    void paint(QPainter *p, const QStyleOptionGraphicsItem *opt, QWidget *w) override {
        QGraphicsRectItem::paint(p, opt, w);
        p->save();
        p->setPen(Qt::white);
        QFont f = p->font(); f.setPointSize(10); f.setBold(true); p->setFont(f);
        p->drawText(rect(), Qt::AlignCenter, m_id);
        p->restore();
    }

private:
    QString m_id;
};

/// Port-style node — like a DAW plugin with labeled audio ports.
/// Inputs on the left, outputs on the right. Anchor metadata carries
/// the port type as a string.
///
/// Usage: construct with a list of input labels and a list of output
/// labels. Port anchor ids are "in:<label>" and "out:<label>".
class PortNode : public QGraphicsRectItem, public Graffodil::IGraphNode {
public:
    PortNode(const QString &id, const QStringList &inputs,
             const QStringList &outputs, const QPointF &pos = {})
        : QGraphicsRectItem(0, 0, 140, 20.0 + 22.0 * qMax(inputs.size(), outputs.size()))
        , m_id(id), m_inputs(inputs), m_outputs(outputs)
    {
        setPos(pos);
        setFlag(ItemIsSelectable);
        setBrush(QColor(100, 100, 110));
        setPen(QPen(QColor(40, 40, 50), 1.5));
        setCursor(Qt::OpenHandCursor);
    }

    QString nodeId() const override { return m_id; }

    QList<Graffodil::Anchor> anchors() const override {
        const QRectF r = mapToScene(rect()).boundingRect();
        QList<Graffodil::Anchor> list;

        const qreal spacing = 22.0;
        for (int i = 0; i < m_inputs.size(); ++i) {
            Graffodil::Anchor a;
            a.id = QStringLiteral("in:%1").arg(m_inputs[i]);
            a.scenePos = {r.left(), r.top() + 14.0 + i * spacing};
            a.outwardDirection = {-1, 0};
            a.metadata = QStringLiteral("audio");
            list.append(a);
        }
        for (int i = 0; i < m_outputs.size(); ++i) {
            Graffodil::Anchor a;
            a.id = QStringLiteral("out:%1").arg(m_outputs[i]);
            a.scenePos = {r.right(), r.top() + 14.0 + i * spacing};
            a.outwardDirection = {1, 0};
            a.metadata = QStringLiteral("audio");
            list.append(a);
        }
        return list;
    }

    QRectF nodeBoundingRect() const override { return mapToScene(rect()).boundingRect(); }
    QGraphicsItem *graphicsItem() override { return this; }

    QRectF boundingRect() const override {
        // Port dots protrude 3px past the rect on each side; include them
        // so the full drawing region gets invalidated on move.
        return QGraphicsRectItem::boundingRect().adjusted(-4, 0, 4, 0);
    }

protected:
    void paint(QPainter *p, const QStyleOptionGraphicsItem *opt, QWidget *w) override {
        QGraphicsRectItem::paint(p, opt, w);
        p->save();
        p->setRenderHint(QPainter::Antialiasing);

        // Title strip
        const QRectF title(rect().left(), rect().top(), rect().width(), 14);
        p->fillRect(title, QColor(60, 60, 70));
        p->setPen(Qt::white);
        QFont tf = p->font(); tf.setPointSize(8); tf.setBold(true); p->setFont(tf);
        p->drawText(title, Qt::AlignCenter, m_id);

        // Port labels & dots (in node-local coords, since we're inside paint)
        const qreal spacing = 22.0;
        p->setPen(QColor(220, 220, 220));
        QFont lf = p->font(); lf.setPointSize(7); lf.setBold(false); p->setFont(lf);

        for (int i = 0; i < m_inputs.size(); ++i) {
            const qreal y = 14.0 + i * spacing;
            const QPointF dot(rect().left(), y);
            p->setBrush(QColor(100, 200, 120));
            p->setPen(QPen(Qt::white, 1));
            p->drawEllipse(dot, 3, 3);
            p->setPen(QColor(220, 220, 220));
            p->drawText(QRectF(rect().left() + 8, y - 6, rect().width() / 2, 12),
                        Qt::AlignLeft | Qt::AlignVCenter, m_inputs[i]);
        }
        for (int i = 0; i < m_outputs.size(); ++i) {
            const qreal y = 14.0 + i * spacing;
            const QPointF dot(rect().right(), y);
            p->setBrush(QColor(200, 130, 100));
            p->setPen(QPen(Qt::white, 1));
            p->drawEllipse(dot, 3, 3);
            p->setPen(QColor(220, 220, 220));
            p->drawText(QRectF(rect().center().x(), y - 6, rect().width() / 2 - 8, 12),
                        Qt::AlignRight | Qt::AlignVCenter, m_outputs[i]);
        }
        p->restore();
    }

private:
    QString m_id;
    QStringList m_inputs;
    QStringList m_outputs;
};

/// Swivel node — circle whose anchor is computed dynamically on demand.
/// The edge attaches to the point on the circle's boundary that's
/// closest to the opposing endpoint. Nodes like this are useful when
/// fixed compass or port anchors don't fit the geometry.
///
/// anchors() returns a single placeholder anchor with empty id;
/// anchorToward(p) computes the real attachment point. Edges built with
/// an empty anchor id on this end are resolved dynamically each adjust.
class SwivelNode : public QGraphicsEllipseItem, public Graffodil::IGraphNode {
public:
    explicit SwivelNode(const QString &id, const QPointF &pos = {})
        : QGraphicsEllipseItem(0, 0, 70, 70), m_id(id)
    {
        setPos(pos);
        setFlag(ItemIsSelectable);
        setBrush(QColor(150, 90, 170));
        setPen(QPen(QColor(90, 50, 110), 1.5));
        setCursor(Qt::OpenHandCursor);
    }

    QString nodeId() const override { return m_id; }

    QList<Graffodil::Anchor> anchors() const override {
        const QRectF r = mapToScene(rect()).boundingRect();
        // Single placeholder anchor at the center. The real attachment
        // happens via anchorToward. Empty id is the signal to
        // CreateEdgeTool and GraphEdgeItem to use dynamic resolution.
        Graffodil::Anchor a;
        a.id = QString();  // empty -> swivel
        a.scenePos = r.center();
        a.outwardDirection = {0, 0};
        return {a};
    }

    Graffodil::Anchor anchorToward(const QPointF &other) const override {
        const QRectF r = mapToScene(rect()).boundingRect();
        const QPointF center = r.center();
        const qreal rx = r.width() / 2.0;
        const qreal ry = r.height() / 2.0;

        QPointF d = other - center;
        qreal len = std::sqrt(d.x() * d.x() + d.y() * d.y());
        if (len < 0.001) d = {1, 0};
        else d = d / len;

        Graffodil::Anchor a;
        a.id = QString();
        a.scenePos = center + QPointF(d.x() * rx, d.y() * ry);
        a.outwardDirection = d;
        return a;
    }

    QRectF nodeBoundingRect() const override { return mapToScene(rect()).boundingRect(); }
    QGraphicsItem *graphicsItem() override { return this; }

protected:
    void paint(QPainter *p, const QStyleOptionGraphicsItem *opt, QWidget *w) override {
        QGraphicsEllipseItem::paint(p, opt, w);
        p->save();
        p->setPen(Qt::white);
        QFont f = p->font(); f.setPointSize(9); f.setBold(true); p->setFont(f);
        p->drawText(rect(), Qt::AlignCenter, m_id);
        p->restore();
    }

private:
    QString m_id;
};
