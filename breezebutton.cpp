/*
 * Copyright 2014  Martin Gräßlin <mgraesslin@kde.org>
 * Copyright 2014  Hugo Pereira Da Costa <hugo.pereira@free.fr>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "breezebutton.h"

#include <KColorScheme>
#include <KColorUtils>
#include <KDecoration3/DecoratedWindow>
//#include <KIconLoader>

#include <QPainter>
#include <QPainterPath>
#include <QVariantAnimation>
#include <QLinearGradient>
#include <QRadialGradient>

namespace Breeze
{
    using KDecoration3::ColorGroup;
    using KDecoration3::ColorRole;
    using KDecoration3::DecorationButtonType;


    //__________________________________________________________________
    Button::Button(DecorationButtonType type, Decoration* decoration, QObject* parent)
        : DecorationButton(type, decoration, parent)
        , m_animation(new QVariantAnimation(this))
    {

        // setup animation
        // It is important start and end value are of the same type, hence 0.0 and not just 0
        m_animation->setStartValue(0.0);
        m_animation->setEndValue(1.0);
        m_animation->setEasingCurve(QEasingCurve::InOutQuad);
        connect(m_animation, &QVariantAnimation::valueChanged, this, [this](const QVariant &value) {
            setOpacity(value.toReal());
        });

        // connections
        connect(decoration->window(), SIGNAL(iconChanged(QIcon)), this, SLOT(update()));
        connect(decoration->settings().get(), &KDecoration3::DecorationSettings::reconfigured, this, &Button::reconfigure);
        connect(this, &KDecoration3::DecorationButton::hoveredChanged, this, &Button::updateAnimationState);

        reconfigure();

    }

    //__________________________________________________________________
    Button::Button(QObject *parent, const QVariantList &args)
        : Button(args.at(0).value<DecorationButtonType>(), args.at(1).value<Decoration*>(), parent)
    {
        setGeometry(QRectF(QPointF(0, 0), preferredSize()));
    }

    //__________________________________________________________________
    Button *Button::create(DecorationButtonType type, KDecoration3::Decoration *decoration, QObject *parent)
    {
        if (auto d = qobject_cast<Decoration*>(decoration))
        {
            Button *b = new Button(type, d, parent);
            const auto c = d->window();
            switch (type)
            {

                case DecorationButtonType::Close:
                b->setVisible(c->isCloseable());
                QObject::connect(c, &KDecoration3::DecoratedWindow::closeableChanged, b, &Breeze::Button::setVisible);
                break;

                case DecorationButtonType::Maximize:
                b->setVisible(c->isMaximizeable());
                QObject::connect(c, &KDecoration3::DecoratedWindow::maximizeableChanged, b, &Breeze::Button::setVisible);
                break;

                case DecorationButtonType::Minimize:
                b->setVisible(c->isMinimizeable());
                QObject::connect(c, &KDecoration3::DecoratedWindow::minimizeableChanged, b, &Breeze::Button::setVisible);
                break;

                case DecorationButtonType::ContextHelp:
                b->setVisible(c->providesContextHelp());
                QObject::connect(c, &KDecoration3::DecoratedWindow::providesContextHelpChanged, b, &Breeze::Button::setVisible);
                break;

                case DecorationButtonType::Shade:
                b->setVisible(c->isShadeable());
                QObject::connect(c, &KDecoration3::DecoratedWindow::shadeableChanged, b, &Breeze::Button::setVisible);
                break;

                case DecorationButtonType::Menu:
                QObject::connect(c, &KDecoration3::DecoratedWindow::iconChanged, b, [b]() { b->update(); });
                break;

                default: break;

            }

            return b;
        }

        return nullptr;

    }

    //__________________________________________________________________
    void Button::paint(QPainter *painter, const QRectF &repaintRegion)
    {
        Q_UNUSED(repaintRegion)

        if (!decoration()) return;

        painter->save();

        // menu button
        if (type() == DecorationButtonType::Menu)
        {
            const QRectF iconRect = geometry().marginsRemoved(m_padding);
            const auto w = decoration()->window();
            /*if (auto deco = qobject_cast<Decoration *>(decoration())) {
                const QPalette activePalette = KIconLoader::global()->customPalette();
                QPalette palette = w->palette();
                palette.setColor(QPalette::WindowText, deco->fontColor());
                KIconLoader::global()->setCustomPalette(palette);
                w->icon().paint(painter, iconRect.toRect());
                if (activePalette == QPalette()) {
                    KIconLoader::global()->resetPalette();
                } else {
                    KIconLoader::global()->setCustomPalette(palette);
                }
            } else {*/
                w->icon().paint(painter, iconRect.toRect());
            //}
        }
        else {

            auto d = qobject_cast<Decoration*>( decoration() );

            if ( d && d->internalSettings()->buttonStyle() == 0 )
                drawIconMacSymbols(painter);
            else if ( d && d->internalSettings()->buttonStyle() == 1 )
                drawIconAqua( painter );
            else if ( d && d->internalSettings()->buttonStyle() == 2 )
                drawIconSunken( painter );
            else if ( d && d->internalSettings()->buttonStyle() == 3 )
                drawIconPlasma( painter );
            else if ( d && d->internalSettings()->buttonStyle() == 4 )
                drawIconOxygen( painter );
            }
        painter->restore();

    }

    //__________________________________________________________________
    void Button::drawIcon(QPainter *painter) const
    {

        painter->setRenderHints(QPainter::Antialiasing);

        /*
        scale painter so that its window matches QRect(-1, -1, 20, 20)
        this makes all further rendering and scaling simpler
        all further rendering is performed inside QRect(0, 0, 18, 18)
        */
        const QRectF rect = geometry().marginsRemoved(m_padding);
        painter->translate(rect.topLeft());

        const qreal width(rect.width());
        painter->scale(width/20, width/20);
        painter->translate(1, 1);

        // render background
        const QColor backgroundColor(this->backgroundColor());

        auto d = qobject_cast<Decoration*>(decoration());
        bool isInactive(d && !d->window()->isActive()
                        && !isHovered() && !isPressed()
                        && m_animation->state() != QAbstractAnimation::Running);
        QColor inactiveCol(Qt::gray);
        if (isInactive)
        {
            int gray = qGray(d->titleBarColor().rgb());
            if (gray <= 200) {
                gray += 55;
                gray = qMax(gray, 115);
            }
            else gray -= 45;
            inactiveCol = QColor(gray, gray, gray);
        }

        // render mark
        const QColor foregroundColor(this->foregroundColor(inactiveCol));
        if (foregroundColor.isValid())
        {

            // setup painter
            QPen pen(foregroundColor);
            pen.setCapStyle(Qt::RoundCap);
            pen.setJoinStyle(Qt::MiterJoin);
            pen.setWidthF(PenWidth::Symbol*qMax((qreal)1.0, 20/width));

            switch (type())
            {

                case DecorationButtonType::Close:
                {
                    QRectF r(0,0, 18, 18);

                    // === Paso 1: fondo liso ===
                    QColor baseColor("#d6dbbf");       // color del botón
                    painter->setBrush(baseColor);
                    painter->setPen(Qt::NoPen);
                    painter->drawEllipse(r);

                    // ===== 2) Sombra interior radial =====
                    // Usamos QRadialGradient pero movemos el foco hacia arriba
                    QRadialGradient radial(
                        r.center().x(),           // centro del degradado
                                           r.center().y() + r.height()*0.10,  // foco desplazado hacia arriba
                                           r.width() / 2.0           // radio
                    );
                    radial.setColorAt(0.0, QColor(0, 0, 0, 0)); // más oscuro en el foco (arriba)
               //     radial.setColorAt(0.6, QColor(0, 0, 0, 10));
                    radial.setColorAt(0.7, QColor(0, 0, 0, 10));
                    radial.setColorAt(1.0, QColor(0, 0, 0, 100));   // se desvanece hacia bordes

                    painter->setBrush(radial);
                    painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
                    painter->drawEllipse(r);

                    // === Paso 3: borde sutil ===
                    QPen border(QColor(0,0,0,100), 1);
                    painter->setPen(border);
                    painter->setBrush(Qt::NoBrush);
                    painter->drawEllipse(r);

                        if (backgroundColor.isValid())
                        {
                            // === Paso 1: fondo liso ===
                            QColor baseColor("#d6dbbf");       // color del botón
                            painter->setBrush(baseColor);
                            painter->setPen(Qt::NoPen);
                            painter->drawEllipse(r);

                            // === Paso 2: sombra interior ===
                            // Creamos un degradado vertical que simule la luz entrando por abajo
                            QLinearGradient shadowGrad(r.topLeft(), r.bottomLeft());
                            shadowGrad.setColorAt(0.0, QColor(0, 0, 0, 90));  // sombra fuerte arriba
                            shadowGrad.setColorAt(0.5, QColor(0, 0, 0, 40));
                            shadowGrad.setColorAt(1.0, QColor(0, 0, 0, 0));   // sin sombra abajo

                            // Usamos composición para "restar luz" (sombra interior)
                            painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
                            painter->setBrush(shadowGrad);
                            painter->drawEllipse(r);

                            // === Paso 3: borde sutil ===
                            QPen border(QColor(0,0,0,120), 1);
                            painter->setPen(border);
                            painter->setBrush(Qt::NoBrush);
                            painter->drawEllipse(r);
                        }

                    break;
                }

                case DecorationButtonType::Maximize:
                {
                        QLinearGradient grad(QPointF(9, 2), QPointF(9, 16));
                        if (d && qGray(d->titleBarColor().rgb()) > 100)
                        {
                            grad.setColorAt(0, isChecked() ? isInactive ? inactiveCol
                                                                        : QColor(67, 198, 176)
                                                           : isInactive ? inactiveCol
                                                                        : QColor(40, 211, 63));
                            grad.setColorAt(1, isChecked() ? isInactive ? inactiveCol
                                                                        : QColor(60, 178, 159)
                                                           : isInactive ? inactiveCol
                                                                        : QColor(36, 191, 57));
                        }
                        else
                        {
                            grad.setColorAt(0, isChecked() ? isInactive ? inactiveCol
                                                                        : QColor(67, 198, 176)
                                                           : isInactive ? inactiveCol
                                                                        : QColor(124, 198, 67));
                            grad.setColorAt(1, isChecked() ? isInactive ? inactiveCol
                                                                        : QColor(60, 178, 159)
                                                           : isInactive ? inactiveCol
                                                                        : QColor(111, 178, 60));
                        }
                        painter->setBrush(QBrush(grad));
                        painter->setPen(Qt::NoPen);
                        painter->drawEllipse(QRectF(1, 1, 16, 16));
                        if (backgroundColor.isValid())
                        {
                            painter->setPen(Qt::NoPen);
                            painter->setBrush(backgroundColor);
                            qreal r = static_cast<qreal>(7)
                                      + (isPressed() ? 0.0
                                         : static_cast<qreal>(2) * m_animation->currentValue().toReal());
                            QPointF c(static_cast<qreal>(9), static_cast<qreal>(9));
                            painter->drawEllipse(c, r, r);
                        }
                    break;
                }

                case DecorationButtonType::Minimize:
                {
                        QLinearGradient grad(QPointF(9, 2), QPointF(9, 16));
                        if (d && qGray(d->titleBarColor().rgb()) > 100)
                        { // yellow isn't good with light backgrounds
                            grad.setColorAt(0, isInactive ? inactiveCol
                                                          : QColor(243, 176, 43));
                            grad.setColorAt(1, isInactive ? inactiveCol
                                                          : QColor(223, 162, 39));
                        }
                        else
                        {
                            grad.setColorAt(0, isInactive ? inactiveCol
                                                          : QColor(237, 198, 81));
                            grad.setColorAt(1, isInactive ? inactiveCol
                                                          : QColor(217, 181, 74));
                        }
                        painter->setBrush(QBrush(grad));
                        painter->setPen(Qt::NoPen);
                        painter->drawEllipse(QRectF(1, 1, 16, 16));
                        if (backgroundColor.isValid())
                        {
                            painter->setPen(Qt::NoPen);
                            painter->setBrush(backgroundColor);
                            qreal r = static_cast<qreal>(7)
                                      + (isPressed() ? 0.0
                                         : static_cast<qreal>(2) * m_animation->currentValue().toReal());
                            QPointF c(static_cast<qreal>(9), static_cast<qreal>(9));
                            painter->drawEllipse(c, r, r);
                        }
                    break;
                }

                case DecorationButtonType::OnAllDesktops:
                {
                    if (!isPressed()) {
                        QLinearGradient grad(QPointF(9, 2), QPointF(9, 16));
                        if (d && qGray(d->titleBarColor().rgb()) > 100)
                        { // yellow isn't good with light backgrounds
                            grad.setColorAt(0, isInactive ? inactiveCol
                                                          : QColor(103, 149, 210));
                            grad.setColorAt(1, isInactive ? inactiveCol
                                                          : QColor(93, 135, 190));
                        }
                        else
                        {
                            grad.setColorAt(0, isInactive ? inactiveCol
                                                          : QColor(135, 166, 220));
                            grad.setColorAt(1, isInactive ? inactiveCol
                                                          : QColor(122, 151, 200));
                        }
                        painter->setBrush(QBrush(grad));
                        painter->setPen(Qt::NoPen);
                        if (isChecked())
                            painter->drawEllipse(QRectF(0, 0, 18, 18));
                        else {
                            painter->drawEllipse(QRectF(1, 1, 16, 16));
                            if (backgroundColor.isValid())
                            {
                                painter->setPen(Qt::NoPen);
                                painter->setBrush(backgroundColor);
                                qreal r = static_cast<qreal>(7)
                                          + static_cast<qreal>(2) * m_animation->currentValue().toReal();
                                QPointF c(static_cast<qreal>(9), static_cast<qreal>(9));
                                painter->drawEllipse(c, r, r);
                            }
                        }
                    }

                    break;
                }

                case DecorationButtonType::Shade:
                {
                    if (!isPressed()) {
                        QLinearGradient grad(QPointF(9, 2), QPointF(9, 16));
                        if (d && qGray(d->titleBarColor().rgb()) > 100)
                        { // yellow isn't good with light backgrounds
                            grad.setColorAt(0, isInactive ? inactiveCol
                                                          : QColor(103, 149, 210));
                            grad.setColorAt(1, isInactive ? inactiveCol
                                                          : QColor(93, 135, 190));
                        }
                        else
                        {
                            grad.setColorAt(0, isInactive ? inactiveCol
                                                          : QColor(135, 166, 220));
                            grad.setColorAt(1, isInactive ? inactiveCol
                                                          : QColor(122, 151, 200));
                        }
                        painter->setBrush(QBrush(grad));
                        painter->setPen(Qt::NoPen);
                        if (isChecked())
                            painter->drawEllipse(QRectF(0, 0, 18, 18));
                        else {
                            painter->drawEllipse(QRectF(1, 1, 16, 16));
                            if (backgroundColor.isValid())
                            {
                                painter->setPen(Qt::NoPen);
                                painter->setBrush(backgroundColor);
                                qreal r = static_cast<qreal>(7)
                                          + static_cast<qreal>(2) * m_animation->currentValue().toReal();
                                QPointF c(static_cast<qreal>(9), static_cast<qreal>(9));
                                painter->drawEllipse(c, r, r);
                            }
                        }
                    }

                    break;

                }

                case DecorationButtonType::KeepBelow:
                {
                    if (!isPressed()) {
                        QLinearGradient grad(QPointF(9, 2), QPointF(9, 16));
                        if (d && qGray(d->titleBarColor().rgb()) > 100)
                        { // yellow isn't good with light backgrounds
                            grad.setColorAt(0, isInactive ? inactiveCol
                                                          : QColor(103, 149, 210));
                            grad.setColorAt(1, isInactive ? inactiveCol
                                                          : QColor(93, 135, 190));
                        }
                        else
                        {
                            grad.setColorAt(0, isInactive ? inactiveCol
                                                          : QColor(135, 166, 220));
                            grad.setColorAt(1, isInactive ? inactiveCol
                                                          : QColor(122, 151, 200));
                        }
                        painter->setBrush(QBrush(grad));
                        painter->setPen(Qt::NoPen);
                        if (isChecked())
                            painter->drawEllipse(QRectF(0, 0, 18, 18));
                        else {
                            painter->drawEllipse(QRectF(1, 1, 16, 16));
                            if (backgroundColor.isValid())
                            {
                                painter->setPen(Qt::NoPen);
                                painter->setBrush(backgroundColor);
                                qreal r = static_cast<qreal>(7)
                                          + static_cast<qreal>(2) * m_animation->currentValue().toReal();
                                QPointF c(static_cast<qreal>(9), static_cast<qreal>(9));
                                painter->drawEllipse(c, r, r);
                            }
                        }
                    }
                    break;

                }

                case DecorationButtonType::KeepAbove:
                {
                    if (!isPressed()) {
                        QLinearGradient grad(QPointF(9, 2), QPointF(9, 16));
                        if (d && qGray(d->titleBarColor().rgb()) > 100)
                        { // yellow isn't good with light backgrounds
                            grad.setColorAt(0, isInactive ? inactiveCol
                                                          : QColor(103, 149, 210));
                            grad.setColorAt(1, isInactive ? inactiveCol
                                                          : QColor(93, 135, 190));
                        }
                        else
                        {
                            grad.setColorAt(0, isInactive ? inactiveCol
                                                          : QColor(135, 166, 220));
                            grad.setColorAt(1, isInactive ? inactiveCol
                                                          : QColor(122, 151, 200));
                        }
                        painter->setBrush(QBrush(grad));
                        painter->setPen(Qt::NoPen);
                        if (isChecked())
                            painter->drawEllipse(QRectF(0, 0, 18, 18));
                        else {
                            painter->drawEllipse(QRectF(1, 1, 16, 16));
                            if (backgroundColor.isValid())
                            {
                                painter->setPen(Qt::NoPen);
                                painter->setBrush(backgroundColor);
                                qreal r = static_cast<qreal>(7)
                                          + static_cast<qreal>(2) * m_animation->currentValue().toReal();
                                QPointF c(static_cast<qreal>(9), static_cast<qreal>(9));
                                painter->drawEllipse(c, r, r);
                            }
                        }
                    }

                    break;
                }


                case DecorationButtonType::ApplicationMenu:
                {
                    if (!isPressed()) {
                        QLinearGradient grad(QPointF(9, 2), QPointF(9, 16));
                        if (d && qGray(d->titleBarColor().rgb()) > 100)
                        { // yellow isn't good with light backgrounds
                            grad.setColorAt(0, isInactive ? inactiveCol
                                                          : QColor(230, 129, 67));
                            grad.setColorAt(1, isInactive ? inactiveCol
                                                          : QColor(210, 118, 61));
                        }
                        else
                        {
                            grad.setColorAt(0, isInactive ? inactiveCol
                                                          : QColor(250, 145, 100));
                            grad.setColorAt(1, isInactive ? inactiveCol
                                                          : QColor(230, 131, 92));
                        }
                        painter->setBrush(QBrush(grad));
                        painter->setPen(Qt::NoPen);
                        painter->drawEllipse(QRectF(1, 1, 16, 16));
                        if (backgroundColor.isValid())
                        {
                            painter->setPen(Qt::NoPen);
                            painter->setBrush(backgroundColor);
                            qreal r = static_cast<qreal>(7)
                                      + static_cast<qreal>(2) * m_animation->currentValue().toReal();
                            QPointF c(static_cast<qreal>(9), static_cast<qreal>(9));
                            painter->drawEllipse(c, r, r);
                        }
                    }

                    break;
                }

                case DecorationButtonType::ContextHelp:
                {
                    if (!isPressed()) {
                        QLinearGradient grad(QPointF(9, 2), QPointF(9, 16));
                        if (d && qGray(d->titleBarColor().rgb()) > 100)
                        { // yellow isn't good with light backgrounds
                            grad.setColorAt(0, isInactive ? inactiveCol
                                                          : QColor(103, 149, 210));
                            grad.setColorAt(1, isInactive ? inactiveCol
                                                          : QColor(93, 135, 190));
                        }
                        else
                        {
                            grad.setColorAt(0, isInactive ? inactiveCol
                                                          : QColor(135, 166, 220));
                            grad.setColorAt(1, isInactive ? inactiveCol
                                                          : QColor(122, 151, 200));
                        }
                        painter->setBrush(QBrush(grad));
                        painter->setPen(Qt::NoPen);
                        painter->drawEllipse(QRectF(1, 1, 16, 16));
                        if (backgroundColor.isValid())
                        {
                            painter->setPen(Qt::NoPen);
                            painter->setBrush(backgroundColor);
                            qreal r = static_cast<qreal>(7)
                                      + static_cast<qreal>(2) * m_animation->currentValue().toReal();
                            QPointF c(static_cast<qreal>(9), static_cast<qreal>(9));
                            painter->drawEllipse(c, r, r);
                        }
                    }

                    break;
                }

                default: break;

            }

        }

    }
    //__________________________________________________________________
    void Button::drawIconPlasma(QPainter *painter) const
    {
        painter->setRenderHints(QPainter::Antialiasing);

        /*
         *    scale painter so that its window matches QRect( -1, -1, 20, 20 )
         *    this makes all further rendering and scaling simpler
         *    all further rendering is performed inside QRect( 0, 0, 18, 18 )
         */
        const QRectF rect = geometry().marginsRemoved(m_padding);
        painter->translate(rect.topLeft());

        const qreal width(rect.width());
        painter->scale(width / 20, width / 20);
        painter->translate(1, 1);

        QColor inactiveCol(Qt::gray);

        // render background
        const QColor backgroundColor(this->backgroundColor());
        if (backgroundColor.isValid()) {
            painter->setPen(Qt::NoPen);
            painter->setBrush(backgroundColor);
            painter->drawEllipse(QRectF(0, 0, 18, 18));
        }

        // render mark
        const QColor foregroundColor(this->foregroundColor(inactiveCol));
        if (foregroundColor.isValid()) {
            // setup painter
            QPen pen(foregroundColor);
            pen.setCapStyle(Qt::RoundCap);
            pen.setJoinStyle(Qt::MiterJoin);
            pen.setWidthF(PenWidth::Symbol * qMax((qreal)1.0, 20 / width));

            painter->setPen(pen);
            painter->setBrush(Qt::NoBrush);

            switch (type()) {
                case DecorationButtonType::Close: {
                    painter->drawLine(QPointF(5, 5), QPointF(13, 13));
                    painter->drawLine(13, 5, 5, 13);
                    break;
                }

                case DecorationButtonType::Maximize: {
                    painter->drawPolyline(QPolygonF()
                    << QPointF(5, 8) << QPointF(5, 13) << QPointF(10, 13));
                    painter->drawPolyline(QPolygonF()
                    << QPointF(8, 5) << QPointF(13, 5) << QPointF(13, 10));
                    break;
                }

                case DecorationButtonType::Minimize: {
                    painter->drawLine(QPointF(4, 9), QPointF(14, 9));
                    break;
                }

                case DecorationButtonType::OnAllDesktops: {
                    painter->setPen(Qt::NoPen);
                    painter->setBrush(foregroundColor);

                    if (isChecked()) {
                        // outer ring
                        painter->drawEllipse(QRectF(3, 3, 12, 12));

                        // center dot
                        QColor backgroundColor(this->backgroundColor());
                        auto d = qobject_cast<Decoration *>(decoration());
                        if (!backgroundColor.isValid() && d) {
                            backgroundColor = d->titleBarColor();
                        }

                        if (backgroundColor.isValid()) {
                            painter->setBrush(backgroundColor);
                            painter->drawEllipse(QRectF(8, 8, 2, 2));
                        }

                    } else {
                        painter->drawPolygon(QVector<QPointF>{QPointF(6.5, 8.5), QPointF(12, 3), QPointF(15, 6), QPointF(9.5, 11.5)});

                        painter->setPen(pen);
                        painter->drawLine(QPointF(5.5, 7.5), QPointF(10.5, 12.5));
                        painter->drawLine(QPointF(12, 6), QPointF(4.5, 13.5));
                    }
                    break;
                }

                case DecorationButtonType::Shade: {
                    if (isChecked()) {
                        painter->drawLine(QPointF(4, 5.5), QPointF(14, 5.5));
                        painter->drawPolyline(QVector<QPointF>{QPointF(4, 8), QPointF(9, 13), QPointF(14, 8)});

                    } else {
                        painter->drawLine(QPointF(4, 5.5), QPointF(14, 5.5));
                        painter->drawPolyline(QVector<QPointF>{QPointF(4, 13), QPointF(9, 8), QPointF(14, 13)});
                    }

                    break;
                }

                case DecorationButtonType::KeepBelow: {
                    painter->drawPolyline(QVector<QPointF>{QPointF(4, 5), QPointF(9, 10), QPointF(14, 5)});

                    painter->drawPolyline(QVector<QPointF>{QPointF(4, 9), QPointF(9, 14), QPointF(14, 9)});
                    break;
                }

                case DecorationButtonType::KeepAbove: {
                    painter->drawPolyline(QVector<QPointF>{QPointF(4, 9), QPointF(9, 4), QPointF(14, 9)});

                    painter->drawPolyline(QVector<QPointF>{QPointF(4, 13), QPointF(9, 8), QPointF(14, 13)});
                    break;
                }

                case DecorationButtonType::ApplicationMenu: {
                    painter->drawRect(QRectF(3.5, 4.5, 11, 1));
                    painter->drawRect(QRectF(3.5, 8.5, 11, 1));
                    painter->drawRect(QRectF(3.5, 12.5, 11, 1));
                    break;
                }

                case DecorationButtonType::ContextHelp: {
                    QPainterPath path;
                    path.moveTo(5, 6);
                    path.arcTo(QRectF(5, 3.5, 8, 5), 180, -180);
                    path.cubicTo(QPointF(12.5, 9.5), QPointF(9, 7.5), QPointF(9, 11.5));
                    painter->drawPath(path);

                    painter->drawRect(QRectF(9, 15, 0.5, 0.5));

                    break;
                }

                default:
                    break;
            }
        }
    }


    //__________________________________________________________________
    void Button::drawIconAqua( QPainter *painter ) const
    {
        painter->setRenderHints(QPainter::Antialiasing);

        /*
         *   scale painter so that its window matches QRect(-1, -1, 20, 20)
         *   this makes all further rendering and scaling simpler
         *   all further rendering is performed inside QRect(0, 0, 18, 18)
         */
        const QRectF rect = geometry().marginsRemoved(m_padding);
        painter->translate(rect.topLeft());

        const qreal width(rect.width());
        painter->scale(width/20, width/20);
        painter->translate(1, 1);

        // render background
        const QColor backgroundColor(this->backgroundColor());

        auto d = qobject_cast<Decoration*>(decoration());
        bool isInactive(d && !d->window()->isActive()
        && !isHovered() && !isPressed()
        && m_animation->state() != QAbstractAnimation::Running);
        QColor inactiveCol(Qt::gray);
        if (isInactive)
        {
            int gray = qGray(d->titleBarColor().rgb());
            if (gray <= 200) {
                gray += 55;
                gray = qMax(gray, 115);
            }
            else gray -= 45;
            inactiveCol = QColor(gray, gray, gray);
        }

        QColor symbolColor;
        symbolColor = QColor(34, 45, 50);
        // render mark
        const QColor foregroundColor(this->foregroundColor(inactiveCol));
        if (foregroundColor.isValid())
        {
            // setup painter
            QPen pen(symbolColor);
            pen.setWidthF(qMax(1.8 * 21 / width, pen.widthF()));

            switch (type())
            {

                case DecorationButtonType::Close:
                {

                    QColor baseColor;

                        if ( !isInactive )
                            baseColor = QColor(255, 92, 87);
                        else
                            baseColor = inactiveCol;

                        QRectF r(0,0, 18, 18);


                        // --- Degradado principal (radial invertido) ---
                        QRadialGradient base(r.center(), r.width()/2, QPointF(r.center().x(), r.bottom()));
                        base.setColorAt(0.0, baseColor.lighter(110));   // parte baja brillante
                        base.setColorAt(0.6, baseColor);
                        base.setColorAt(1.0, baseColor.darker(110));    // borde oscuro
                        painter->setBrush(base);
                        painter->setPen(baseColor.darker(140));
                        painter->drawEllipse(r);

                        // --- Highlight superior ovalado (reflejo Aqua) ---
                        QRectF highlightRect(r.left()+4, r.top()+1, r.width()-8, r.height()/2.5);
                        QLinearGradient gloss(highlightRect.topLeft(), highlightRect.bottomLeft());
                        gloss.setColorAt(0.0, QColor(255,255,255,180));
                        gloss.setColorAt(1.0, QColor(255,255,255,0));
                        painter->setBrush(gloss);
                        painter->setPen(Qt::NoPen);
                        painter->drawEllipse(highlightRect);

                        // --- Bisel interior claro ---
                        QRectF highlightRectb(r.left()+2, r.top()+r.height()/1.9, r.width()-4, r.height()/2.2);
                        QRadialGradient innerHighlight(r.center(), r.width()/2, r.center());
                        innerHighlight.setColorAt(0.0, QColor(255, 255, 255, 0));
                        innerHighlight.setColorAt(1.0, QColor(255, 255, 255, 55));
                        painter->setBrush(innerHighlight);
                        painter->setPen(Qt::NoPen);
                        painter->drawEllipse(highlightRectb);

                        if (backgroundColor.isValid())
                        {
                            QRectF r(0,0, 18, 18);

                            // --- Degradado principal (radial invertido) ---
                            QRadialGradient base(r.center(), r.width()/2, QPointF(r.center().x(), r.bottom()));
                            base.setColorAt(0.0, baseColor.lighter(110));   // parte baja brillante
                            base.setColorAt(0.6, baseColor);
                            base.setColorAt(1.0, baseColor.darker(110));    // borde oscuro
                            painter->setBrush(base);
                            painter->setPen(baseColor.darker(140));
                            painter->drawEllipse(r);

                            // --- Highlight superior ovalado (reflejo Aqua) ---
                            QRectF highlightRect(r.left()+4, r.top()+1, r.width()-8, r.height()/2.5);
                            QLinearGradient gloss(highlightRect.topLeft(), highlightRect.bottomLeft());
                            gloss.setColorAt(0.0, QColor(255,255,255,180));
                            gloss.setColorAt(1.0, QColor(255,255,255,0));
                            painter->setBrush(gloss);
                            painter->setPen(Qt::NoPen);
                            painter->drawEllipse(highlightRect);

                            // --- Bisel interior claro ---
                            QRectF highlightRectb(r.left()+2, r.top()+r.height()/1.9, r.width()-4, r.height()/2.2);
                            QRadialGradient innerHighlight(r.center(), r.width()/2, r.center());
                            innerHighlight.setColorAt(0.0, QColor(255, 255, 255, 0));
                            innerHighlight.setColorAt(1.0, QColor(255, 255, 255, 55));
                            painter->setBrush(innerHighlight);
                            painter->setPen(Qt::NoPen);
                            painter->drawEllipse(highlightRectb);

                        }
                        if (isHovered()) {
                            painter->setPen(pen);
                            painter->setBrush(symbolColor);

                            painter->drawLine( QPointF( 6, 6 ), QPointF( 12, 12 ) );
                            painter->drawLine( QPointF( 6, 12 ), QPointF( 12, 6 ) );
                        }
                    break;
                }

                case DecorationButtonType::Maximize:
                {
                        QColor baseColor;

                        if ( !isInactive )
                            baseColor = QColor(36, 191, 57);
                        else
                            baseColor = inactiveCol;

                        QRectF r(0,0, 18, 18);

                        // --- Degradado principal (radial invertido) ---
                        QRadialGradient base(r.center(), r.width()/2, QPointF(r.center().x(), r.bottom()));
                        base.setColorAt(0.0, baseColor.lighter(110));   // parte baja brillante
                        base.setColorAt(0.6, baseColor);
                        base.setColorAt(1.0, baseColor.darker(110));    // borde oscuro
                        painter->setBrush(base);
                        painter->setPen(baseColor.darker(140));
                        painter->drawEllipse(r);

                        // --- Highlight superior ovalado (reflejo Aqua) ---
                        QRectF highlightRect(r.left()+4, r.top()+1, r.width()-8, r.height()/2.5);
                        QLinearGradient gloss(highlightRect.topLeft(), highlightRect.bottomLeft());
                        gloss.setColorAt(0.0, QColor(255,255,255,180));
                        gloss.setColorAt(1.0, QColor(255,255,255,0));
                        painter->setBrush(gloss);
                        painter->setPen(Qt::NoPen);
                        painter->drawEllipse(highlightRect);

                        // --- Bisel interior claro ---
                        QRectF highlightRectb(r.left()+2, r.top()+r.height()/1.9, r.width()-4, r.height()/2.2);
                        QRadialGradient innerHighlight(r.center(), r.width()/2, r.center());
                        innerHighlight.setColorAt(0.0, QColor(255, 255, 255, 0));
                        innerHighlight.setColorAt(1.0, QColor(255, 255, 255, 55));
                        painter->setBrush(innerHighlight);
                        painter->setPen(Qt::NoPen);
                        painter->drawEllipse(highlightRectb);

                        if (backgroundColor.isValid())
                        {

                            QRectF r(0,0, 18, 18);

                            // --- Degradado principal (radial invertido) ---
                            QRadialGradient base(r.center(), r.width()/2, QPointF(r.center().x(), r.bottom()));
                            base.setColorAt(0.0, baseColor.lighter(110));   // parte baja brillante
                            base.setColorAt(0.6, baseColor);
                            base.setColorAt(1.0, baseColor.darker(110));    // borde oscuro
                            painter->setBrush(base);
                            painter->setPen(baseColor.darker(140));
                            painter->drawEllipse(r);

                            // --- Highlight superior ovalado (reflejo Aqua) ---
                            QRectF highlightRect(r.left()+4, r.top()+1, r.width()-8, r.height()/2.5);
                            QLinearGradient gloss(highlightRect.topLeft(), highlightRect.bottomLeft());
                            gloss.setColorAt(0.0, QColor(255,255,255,180));
                            gloss.setColorAt(1.0, QColor(255,255,255,0));
                            painter->setBrush(gloss);
                            painter->setPen(Qt::NoPen);
                            painter->drawEllipse(highlightRect);

                            // --- Bisel interior claro ---
                            QRectF highlightRectb(r.left()+2, r.top()+r.height()/1.9, r.width()-4, r.height()/2.2);
                            QRadialGradient innerHighlight(r.center(), r.width()/2, r.center());
                            innerHighlight.setColorAt(0.0, QColor(255, 255, 255, 0));
                            innerHighlight.setColorAt(1.0, QColor(255, 255, 255, 55));
                            painter->setBrush(innerHighlight);
                            painter->setPen(Qt::NoPen);
                            painter->drawEllipse(highlightRectb);
                        }
                        if (isHovered()) {
                            painter->setPen( Qt::NoPen );

                            // two triangles
                            QPainterPath path1, path2;
                            if( isChecked() )
                            {
                                path1.moveTo(8.5, 9.5);
                                path1.lineTo(2.5, 9.5);
                                path1.lineTo(8.5, 15.5);

                                path2.moveTo(9.5, 8.5);
                                path2.lineTo(15.5, 8.5);
                                path2.lineTo(9.5, 2.5);
                            }
                            else
                            {
                                path1.moveTo(5, 13);
                                path1.lineTo(11, 13);
                                path1.lineTo(5, 7);

                                path2.moveTo(13, 5);
                                path2.lineTo(7, 5);
                                path2.lineTo(13, 11);
                            }

                            painter->fillPath(path1, QBrush(symbolColor));
                            painter->fillPath(path2, QBrush(symbolColor));
                        }
                    break;
                }

                case DecorationButtonType::Minimize:
                {
                        QColor baseColor;

                        if ( !isInactive )
                            baseColor = QColor(243, 176, 43);
                        else
                            baseColor = inactiveCol;

                        QRectF r(0,0, 18, 18);

                        // --- Degradado principal (radial invertido) ---
                        QRadialGradient base(r.center(), r.width()/2, QPointF(r.center().x(), r.bottom()));
                        base.setColorAt(0.0, baseColor.lighter(110));   // parte baja brillante
                        base.setColorAt(0.6, baseColor);
                        base.setColorAt(1.0, baseColor.darker(110));    // borde oscuro
                        painter->setBrush(base);
                        painter->setPen(baseColor.darker(140));
                        painter->drawEllipse(r);

                        // --- Highlight superior ovalado (reflejo Aqua) ---
                        QRectF highlightRect(r.left()+4, r.top()+1, r.width()-8, r.height()/2.5);
                        QLinearGradient gloss(highlightRect.topLeft(), highlightRect.bottomLeft());
                        gloss.setColorAt(0.0, QColor(255,255,255,180));
                        gloss.setColorAt(1.0, QColor(255,255,255,0));
                        painter->setBrush(gloss);
                        painter->setPen(Qt::NoPen);
                        painter->drawEllipse(highlightRect);

                        // --- Bisel interior claro ---
                        QRectF highlightRectb(r.left()+2, r.top()+r.height()/1.9, r.width()-4, r.height()/2.2);
                        QRadialGradient innerHighlight(r.center(), r.width()/2, r.center());
                        innerHighlight.setColorAt(0.0, QColor(255, 255, 255, 0));
                        innerHighlight.setColorAt(1.0, QColor(255, 255, 255, 55));
                        painter->setBrush(innerHighlight);
                        painter->setPen(Qt::NoPen);
                        painter->drawEllipse(highlightRectb);

                        if (backgroundColor.isValid())
                        {

                            QRectF r(0,0, 18, 18);

                            // --- Degradado principal (radial invertido) ---
                            QRadialGradient base(r.center(), r.width()/2, QPointF(r.center().x(), r.bottom()));
                            base.setColorAt(0.0, baseColor.lighter(110));   // parte baja brillante
                            base.setColorAt(0.6, baseColor);
                            base.setColorAt(1.0, baseColor.darker(110));    // borde oscuro
                            painter->setBrush(base);
                            painter->setPen(baseColor.darker(140));
                            painter->drawEllipse(r);

                            // --- Highlight superior ovalado (reflejo Aqua) ---
                            QRectF highlightRect(r.left()+4, r.top()+1, r.width()-8, r.height()/2.5);
                            QLinearGradient gloss(highlightRect.topLeft(), highlightRect.bottomLeft());
                            gloss.setColorAt(0.0, QColor(255,255,255,180));
                            gloss.setColorAt(1.0, QColor(255,255,255,0));
                            painter->setBrush(gloss);
                            painter->setPen(Qt::NoPen);
                            painter->drawEllipse(highlightRect);

                            // --- Bisel interior claro ---
                            QRectF highlightRectb(r.left()+2, r.top()+r.height()/1.9, r.width()-4, r.height()/2.2);
                            QRadialGradient innerHighlight(r.center(), r.width()/2, r.center());
                            innerHighlight.setColorAt(0.0, QColor(255, 255, 255, 0));
                            innerHighlight.setColorAt(1.0, QColor(255, 255, 255, 55));
                            painter->setBrush(innerHighlight);
                            painter->setPen(Qt::NoPen);
                            painter->drawEllipse(highlightRectb);
                        }
                        if (isHovered()) {
                            pen.setWidthF(1.2*qMax((qreal)1.0, 20/width));
                            painter->setPen(pen);
                            painter->setBrush(symbolColor);
                            painter->drawLine( QPointF( 5, 9 ), QPointF( 13, 9 ) );
                        }
                    break;
                }

                case DecorationButtonType::OnAllDesktops:
                {
                    QColor baseColor;

                    if ( !isInactive )
                        baseColor = QColor(103, 149, 210);
                    else
                        baseColor = inactiveCol;

                    QRectF r(0,0, 18, 18);

                    // --- Degradado principal (radial invertido) ---
                    QRadialGradient base(r.center(), r.width()/2, QPointF(r.center().x(), r.bottom()));
                    base.setColorAt(0.0, baseColor.lighter(110));   // parte baja brillante
                    base.setColorAt(0.6, baseColor);
                    base.setColorAt(1.0, baseColor.darker(110));    // borde oscuro
                    painter->setBrush(base);
                    painter->setPen(baseColor.darker(140));
                    painter->drawEllipse(r);

                    // --- Highlight superior ovalado (reflejo Aqua) ---
                    QRectF highlightRect(r.left()+4, r.top()+1, r.width()-8, r.height()/2.5);
                    QLinearGradient gloss(highlightRect.topLeft(), highlightRect.bottomLeft());
                    gloss.setColorAt(0.0, QColor(255,255,255,180));
                    gloss.setColorAt(1.0, QColor(255,255,255,0));
                    painter->setBrush(gloss);
                    painter->setPen(Qt::NoPen);
                    painter->drawEllipse(highlightRect);

                    // --- Bisel interior claro ---
                    QRectF highlightRectb(r.left()+2, r.top()+r.height()/1.9, r.width()-4, r.height()/2.2);
                    QRadialGradient innerHighlight(r.center(), r.width()/2, r.center());
                    innerHighlight.setColorAt(0.0, QColor(255, 255, 255, 0));
                    innerHighlight.setColorAt(1.0, QColor(255, 255, 255, 55));
                    painter->setBrush(innerHighlight);
                    painter->setPen(Qt::NoPen);
                    painter->drawEllipse(highlightRectb);
                    //  painter->drawEllipse(QRectF(1, 1, 16, 16));
                    if (backgroundColor.isValid())
                    {
                        QRectF r(0,0, 18, 18);

                        // --- Degradado principal (radial invertido) ---
                        QRadialGradient base(r.center(), r.width()/2, QPointF(r.center().x(), r.bottom()));
                        base.setColorAt(0.0, baseColor.lighter(110));   // parte baja brillante
                        base.setColorAt(0.6, baseColor);
                        base.setColorAt(1.0, baseColor.darker(110));    // borde oscuro
                        painter->setBrush(base);
                        painter->setPen(baseColor.darker(140));
                        painter->drawEllipse(r);

                        // --- Highlight superior ovalado (reflejo Aqua) ---
                        QRectF highlightRect(r.left()+4, r.top()+1, r.width()-8, r.height()/2.5);
                        QLinearGradient gloss(highlightRect.topLeft(), highlightRect.bottomLeft());
                        gloss.setColorAt(0.0, QColor(255,255,255,180));
                        gloss.setColorAt(1.0, QColor(255,255,255,0));
                        painter->setBrush(gloss);
                        painter->setPen(Qt::NoPen);
                        painter->drawEllipse(highlightRect);

                        // --- Bisel interior claro ---
                        QRectF highlightRectb(r.left()+2, r.top()+r.height()/1.9, r.width()-4, r.height()/2.2);
                        QRadialGradient innerHighlight(r.center(), r.width()/2, r.center());
                        innerHighlight.setColorAt(0.0, QColor(255, 255, 255, 0));
                        innerHighlight.setColorAt(1.0, QColor(255, 255, 255, 55));
                        painter->setBrush(innerHighlight);
                        painter->setPen(Qt::NoPen);
                        painter->drawEllipse(highlightRectb);
                    }
                    if (isPressed() || isHovered() || isChecked()) {
                        /*         if ((isPressed()) && backgroundColor.isValid())
                         { * * *
                         painter->setPen(Qt::NoPen);
                         painter->setBrush(backgroundColor);
                         painter->drawEllipse(QRectF(0, 0, 18, 18));
                    } */
                        painter->setPen(pen);
                        painter->setBrush(Qt::NoBrush);
                        painter->drawEllipse(QRectF(6, 6, 6, 6));
                    }
                break;
                }

                case DecorationButtonType::Shade:
                {
                    QColor baseColor;

                    if ( !isInactive )
                        baseColor = QColor(103, 149, 210);
                    else
                        baseColor = inactiveCol;

                    QRectF r(0,0, 18, 18);

                    // --- Degradado principal (radial invertido) ---
                    QRadialGradient base(r.center(), r.width()/2, QPointF(r.center().x(), r.bottom()));
                    base.setColorAt(0.0, baseColor.lighter(110));   // parte baja brillante
                    base.setColorAt(0.6, baseColor);
                    base.setColorAt(1.0, baseColor.darker(110));    // borde oscuro
                    painter->setBrush(base);
                    painter->setPen(baseColor.darker(140));
                    painter->drawEllipse(r);

                    // --- Highlight superior ovalado (reflejo Aqua) ---
                    QRectF highlightRect(r.left()+4, r.top()+1, r.width()-8, r.height()/2.5);
                    QLinearGradient gloss(highlightRect.topLeft(), highlightRect.bottomLeft());
                    gloss.setColorAt(0.0, QColor(255,255,255,180));
                    gloss.setColorAt(1.0, QColor(255,255,255,0));
                    painter->setBrush(gloss);
                    painter->setPen(Qt::NoPen);
                    painter->drawEllipse(highlightRect);

                    // --- Bisel interior claro ---
                    QRectF highlightRectb(r.left()+2, r.top()+r.height()/1.9, r.width()-4, r.height()/2.2);
                    QRadialGradient innerHighlight(r.center(), r.width()/2, r.center());
                    innerHighlight.setColorAt(0.0, QColor(255, 255, 255, 0));
                    innerHighlight.setColorAt(1.0, QColor(255, 255, 255, 55));
                    painter->setBrush(innerHighlight);
                    painter->setPen(Qt::NoPen);
                    painter->drawEllipse(highlightRectb);
                    //  painter->drawEllipse(QRectF(1, 1, 16, 16));
                    if (backgroundColor.isValid())
                    {
                        QRectF r(0,0, 18, 18);

                        // --- Degradado principal (radial invertido) ---
                        QRadialGradient base(r.center(), r.width()/2, QPointF(r.center().x(), r.bottom()));
                        base.setColorAt(0.0, baseColor.lighter(110));   // parte baja brillante
                        base.setColorAt(0.6, baseColor);
                        base.setColorAt(1.0, baseColor.darker(110));    // borde oscuro
                        painter->setBrush(base);
                        painter->setPen(baseColor.darker(140));
                        painter->drawEllipse(r);

                        // --- Highlight superior ovalado (reflejo Aqua) ---
                        QRectF highlightRect(r.left()+4, r.top()+1, r.width()-8, r.height()/2.5);
                        QLinearGradient gloss(highlightRect.topLeft(), highlightRect.bottomLeft());
                        gloss.setColorAt(0.0, QColor(255,255,255,180));
                        gloss.setColorAt(1.0, QColor(255,255,255,0));
                        painter->setBrush(gloss);
                        painter->setPen(Qt::NoPen);
                        painter->drawEllipse(highlightRect);

                        // --- Bisel interior claro ---
                        QRectF highlightRectb(r.left()+2, r.top()+r.height()/1.9, r.width()-4, r.height()/2.2);
                        QRadialGradient innerHighlight(r.center(), r.width()/2, r.center());
                        innerHighlight.setColorAt(0.0, QColor(255, 255, 255, 0));
                        innerHighlight.setColorAt(1.0, QColor(255, 255, 255, 55));
                        painter->setBrush(innerHighlight);
                        painter->setPen(Qt::NoPen);
                        painter->drawEllipse(highlightRectb);
                    }
                    if (isPressed() || isHovered() || isChecked()) {
                        /*         if ((isPressed()) && backgroundColor.isValid())
                         { * *
                         painter->setPen(Qt::NoPen);
                         painter->setBrush(backgroundColor);
                         painter->drawEllipse(QRectF(0, 0, 18, 18));
                    } */
                        painter->setPen(pen);
                        painter->setBrush(Qt::NoBrush);
                        painter->drawPolyline(QPolygonF()
                        << QPointF(5, 13)
                        << QPointF(9, 9)
                        << QPointF(13, 13));
                    }
                    break;

                }

                case DecorationButtonType::KeepBelow:
                {
                    QColor baseColor;

                    if ( !isInactive )
                        baseColor = QColor(103, 149, 210);
                    else
                        baseColor = inactiveCol;

                    QRectF r(0,0, 18, 18);

                    // --- Degradado principal (radial invertido) ---
                    QRadialGradient base(r.center(), r.width()/2, QPointF(r.center().x(), r.bottom()));
                    base.setColorAt(0.0, baseColor.lighter(110));   // parte baja brillante
                    base.setColorAt(0.6, baseColor);
                    base.setColorAt(1.0, baseColor.darker(110));    // borde oscuro
                    painter->setBrush(base);
                    painter->setPen(baseColor.darker(140));
                    painter->drawEllipse(r);

                    // --- Highlight superior ovalado (reflejo Aqua) ---
                    QRectF highlightRect(r.left()+4, r.top()+1, r.width()-8, r.height()/2.5);
                    QLinearGradient gloss(highlightRect.topLeft(), highlightRect.bottomLeft());
                    gloss.setColorAt(0.0, QColor(255,255,255,180));
                    gloss.setColorAt(1.0, QColor(255,255,255,0));
                    painter->setBrush(gloss);
                    painter->setPen(Qt::NoPen);
                    painter->drawEllipse(highlightRect);

                    // --- Bisel interior claro ---
                    QRectF highlightRectb(r.left()+2, r.top()+r.height()/1.9, r.width()-4, r.height()/2.2);
                    QRadialGradient innerHighlight(r.center(), r.width()/2, r.center());
                    innerHighlight.setColorAt(0.0, QColor(255, 255, 255, 0));
                    innerHighlight.setColorAt(1.0, QColor(255, 255, 255, 55));
                    painter->setBrush(innerHighlight);
                    painter->setPen(Qt::NoPen);
                    painter->drawEllipse(highlightRectb);
                    //  painter->drawEllipse(QRectF(1, 1, 16, 16));
                    if (backgroundColor.isValid())
                    {
                        QRectF r(0,0, 18, 18);

                        // --- Degradado principal (radial invertido) ---
                        QRadialGradient base(r.center(), r.width()/2, QPointF(r.center().x(), r.bottom()));
                        base.setColorAt(0.0, baseColor.lighter(110));   // parte baja brillante
                        base.setColorAt(0.6, baseColor);
                        base.setColorAt(1.0, baseColor.darker(110));    // borde oscuro
                        painter->setBrush(base);
                        painter->setPen(baseColor.darker(140));
                        painter->drawEllipse(r);

                        // --- Highlight superior ovalado (reflejo Aqua) ---
                        QRectF highlightRect(r.left()+4, r.top()+1, r.width()-8, r.height()/2.5);
                        QLinearGradient gloss(highlightRect.topLeft(), highlightRect.bottomLeft());
                        gloss.setColorAt(0.0, QColor(255,255,255,180));
                        gloss.setColorAt(1.0, QColor(255,255,255,0));
                        painter->setBrush(gloss);
                        painter->setPen(Qt::NoPen);
                        painter->drawEllipse(highlightRect);

                        // --- Bisel interior claro ---
                        QRectF highlightRectb(r.left()+2, r.top()+r.height()/1.9, r.width()-4, r.height()/2.2);
                        QRadialGradient innerHighlight(r.center(), r.width()/2, r.center());
                        innerHighlight.setColorAt(0.0, QColor(255, 255, 255, 0));
                        innerHighlight.setColorAt(1.0, QColor(255, 255, 255, 55));
                        painter->setBrush(innerHighlight);
                        painter->setPen(Qt::NoPen);
                        painter->drawEllipse(highlightRectb);
                    }
                    if (isPressed() || isHovered() || isChecked()) {
                        /*         if ((isPressed()) && backgroundColor.isValid())
                         { *
                         painter->setPen(Qt::NoPen);
                         painter->setBrush(backgroundColor);
                         painter->drawEllipse(QRectF(0, 0, 18, 18));
                    } */
                        painter->setPen(pen);
                        painter->setBrush(Qt::NoBrush);
                        painter->drawPolyline(QPolygonF()
                        << QPointF(6, 6)
                        << QPointF(9, 9)
                        << QPointF(12, 6));

                        painter->drawPolyline(QPolygonF()
                        << QPointF(6, 10)
                        << QPointF(9, 13)
                        << QPointF(12, 10));
                    }
                    break;

                }

                case DecorationButtonType::KeepAbove:
                {
                        QColor baseColor;

                        if ( !isInactive )
                            baseColor = QColor(103, 149, 210);
                        else
                            baseColor = inactiveCol;

                        QRectF r(0,0, 18, 18);

                        // --- Degradado principal (radial invertido) ---
                        QRadialGradient base(r.center(), r.width()/2, QPointF(r.center().x(), r.bottom()));
                        base.setColorAt(0.0, baseColor.lighter(110));   // parte baja brillante
                        base.setColorAt(0.6, baseColor);
                        base.setColorAt(1.0, baseColor.darker(110));    // borde oscuro
                        painter->setBrush(base);
                        painter->setPen(baseColor.darker(140));
                        painter->drawEllipse(r);

                        // --- Highlight superior ovalado (reflejo Aqua) ---
                        QRectF highlightRect(r.left()+4, r.top()+1, r.width()-8, r.height()/2.5);
                        QLinearGradient gloss(highlightRect.topLeft(), highlightRect.bottomLeft());
                        gloss.setColorAt(0.0, QColor(255,255,255,180));
                        gloss.setColorAt(1.0, QColor(255,255,255,0));
                        painter->setBrush(gloss);
                        painter->setPen(Qt::NoPen);
                        painter->drawEllipse(highlightRect);

                        // --- Bisel interior claro ---
                        QRectF highlightRectb(r.left()+2, r.top()+r.height()/1.9, r.width()-4, r.height()/2.2);
                        QRadialGradient innerHighlight(r.center(), r.width()/2, r.center());
                        innerHighlight.setColorAt(0.0, QColor(255, 255, 255, 0));
                        innerHighlight.setColorAt(1.0, QColor(255, 255, 255, 55));
                        painter->setBrush(innerHighlight);
                        painter->setPen(Qt::NoPen);
                        painter->drawEllipse(highlightRectb);
                          //  painter->drawEllipse(QRectF(1, 1, 16, 16));
                            if (backgroundColor.isValid())
                            {
                                QRectF r(0,0, 18, 18);

                                // --- Degradado principal (radial invertido) ---
                                QRadialGradient base(r.center(), r.width()/2, QPointF(r.center().x(), r.bottom()));
                                base.setColorAt(0.0, baseColor.lighter(110));   // parte baja brillante
                                base.setColorAt(0.6, baseColor);
                                base.setColorAt(1.0, baseColor.darker(110));    // borde oscuro
                                painter->setBrush(base);
                                painter->setPen(baseColor.darker(140));
                                painter->drawEllipse(r);

                                // --- Highlight superior ovalado (reflejo Aqua) ---
                                QRectF highlightRect(r.left()+4, r.top()+1, r.width()-8, r.height()/2.5);
                                QLinearGradient gloss(highlightRect.topLeft(), highlightRect.bottomLeft());
                                gloss.setColorAt(0.0, QColor(255,255,255,180));
                                gloss.setColorAt(1.0, QColor(255,255,255,0));
                                painter->setBrush(gloss);
                                painter->setPen(Qt::NoPen);
                                painter->drawEllipse(highlightRect);

                                // --- Bisel interior claro ---
                                QRectF highlightRectb(r.left()+2, r.top()+r.height()/1.9, r.width()-4, r.height()/2.2);
                                QRadialGradient innerHighlight(r.center(), r.width()/2, r.center());
                                innerHighlight.setColorAt(0.0, QColor(255, 255, 255, 0));
                                innerHighlight.setColorAt(1.0, QColor(255, 255, 255, 55));
                                painter->setBrush(innerHighlight);
                                painter->setPen(Qt::NoPen);
                                painter->drawEllipse(highlightRectb);
                            }
                    if (isPressed() || isHovered() || isChecked()) {
               /*         if ((isPressed()) && backgroundColor.isValid())
                        {
                            painter->setPen(Qt::NoPen);
                            painter->setBrush(backgroundColor);
                            painter->drawEllipse(QRectF(0, 0, 18, 18));
                        } */
                        painter->setPen(pen);
                        painter->setBrush(Qt::NoBrush);
                        painter->drawPolyline(QPolygonF()
                        << QPointF(6, 8)
                        << QPointF(9, 5)
                        << QPointF(12, 8));

                        painter->drawPolyline(QPolygonF()
                        << QPointF(6, 12)
                        << QPointF(9, 9)
                        << QPointF(12, 12));
                    }
                    break;
                }


                case DecorationButtonType::ApplicationMenu:
                {
                    QColor baseColor;

                    if ( !isInactive )
                        baseColor = QColor(230, 129, 67);
                    else
                        baseColor = inactiveCol;

                    QRectF r(0,0, 18, 18);

                    // --- Degradado principal (radial invertido) ---
                    QRadialGradient base(r.center(), r.width()/2, QPointF(r.center().x(), r.bottom()));
                    base.setColorAt(0.0, baseColor.lighter(110));   // parte baja brillante
                    base.setColorAt(0.6, baseColor);
                    base.setColorAt(1.0, baseColor.darker(110));    // borde oscuro
                    painter->setBrush(base);
                    painter->setPen(baseColor.darker(140));
                    painter->drawEllipse(r);

                    // --- Highlight superior ovalado (reflejo Aqua) ---
                    QRectF highlightRect(r.left()+4, r.top()+1, r.width()-8, r.height()/2.5);
                    QLinearGradient gloss(highlightRect.topLeft(), highlightRect.bottomLeft());
                    gloss.setColorAt(0.0, QColor(255,255,255,180));
                    gloss.setColorAt(1.0, QColor(255,255,255,0));
                    painter->setBrush(gloss);
                    painter->setPen(Qt::NoPen);
                    painter->drawEllipse(highlightRect);

                    // --- Bisel interior claro ---
                    QRectF highlightRectb(r.left()+2, r.top()+r.height()/1.9, r.width()-4, r.height()/2.2);
                    QRadialGradient innerHighlight(r.center(), r.width()/2, r.center());
                    innerHighlight.setColorAt(0.0, QColor(255, 255, 255, 0));
                    innerHighlight.setColorAt(1.0, QColor(255, 255, 255, 55));
                    painter->setBrush(innerHighlight);
                    painter->setPen(Qt::NoPen);
                    painter->drawEllipse(highlightRectb);
                    //  painter->drawEllipse(QRectF(1, 1, 16, 16));
                    if (backgroundColor.isValid())
                    {
                        QRectF r(0,0, 18, 18);

                        // --- Degradado principal (radial invertido) ---
                        QRadialGradient base(r.center(), r.width()/2, QPointF(r.center().x(), r.bottom()));
                        base.setColorAt(0.0, baseColor.lighter(110));   // parte baja brillante
                        base.setColorAt(0.6, baseColor);
                        base.setColorAt(1.0, baseColor.darker(110));    // borde oscuro
                        painter->setBrush(base);
                        painter->setPen(baseColor.darker(140));
                        painter->drawEllipse(r);

                        // --- Highlight superior ovalado (reflejo Aqua) ---
                        QRectF highlightRect(r.left()+4, r.top()+1, r.width()-8, r.height()/2.5);
                        QLinearGradient gloss(highlightRect.topLeft(), highlightRect.bottomLeft());
                        gloss.setColorAt(0.0, QColor(255,255,255,180));
                        gloss.setColorAt(1.0, QColor(255,255,255,0));
                        painter->setBrush(gloss);
                        painter->setPen(Qt::NoPen);
                        painter->drawEllipse(highlightRect);

                        // --- Bisel interior claro ---
                        QRectF highlightRectb(r.left()+2, r.top()+r.height()/1.9, r.width()-4, r.height()/2.2);
                        QRadialGradient innerHighlight(r.center(), r.width()/2, r.center());
                        innerHighlight.setColorAt(0.0, QColor(255, 255, 255, 0));
                        innerHighlight.setColorAt(1.0, QColor(255, 255, 255, 55));
                        painter->setBrush(innerHighlight);
                        painter->setPen(Qt::NoPen);
                        painter->drawEllipse(highlightRectb);
                    }
                    if (isPressed() || isHovered()) {
                        /*         if ((isPressed()) && backgroundColor.isValid())
                         { *
                         painter->setPen(Qt::NoPen);
                         painter->setBrush(backgroundColor);
                         painter->drawEllipse(QRectF(0, 0, 18, 18));
                    } */
                        painter->setPen(pen);
                        painter->setBrush(Qt::NoBrush);
                        painter->drawLine(QPointF(4.5, 6), QPointF(13.5, 6));
                        painter->drawLine(QPointF(4.5, 9), QPointF(13.5, 9));
                        painter->drawLine(QPointF(4.5, 12), QPointF(13.5, 12));
                    }
                    break;
                }

                case DecorationButtonType::ContextHelp:
                {
                    QColor baseColor;

                    if ( !isInactive )
                        baseColor = QColor(230, 129, 67);
                    else
                        baseColor = inactiveCol;

                    QRectF r(0,0, 18, 18);

                    // --- Degradado principal (radial invertido) ---
                    QRadialGradient base(r.center(), r.width()/2, QPointF(r.center().x(), r.bottom()));
                    base.setColorAt(0.0, baseColor.lighter(110));   // parte baja brillante
                    base.setColorAt(0.6, baseColor);
                    base.setColorAt(1.0, baseColor.darker(110));    // borde oscuro
                    painter->setBrush(base);
                    painter->setPen(baseColor.darker(140));
                    painter->drawEllipse(r);

                    // --- Highlight superior ovalado (reflejo Aqua) ---
                    QRectF highlightRect(r.left()+4, r.top()+1, r.width()-8, r.height()/2.5);
                    QLinearGradient gloss(highlightRect.topLeft(), highlightRect.bottomLeft());
                    gloss.setColorAt(0.0, QColor(255,255,255,180));
                    gloss.setColorAt(1.0, QColor(255,255,255,0));
                    painter->setBrush(gloss);
                    painter->setPen(Qt::NoPen);
                    painter->drawEllipse(highlightRect);

                    // --- Bisel interior claro ---
                    QRectF highlightRectb(r.left()+2, r.top()+r.height()/1.9, r.width()-4, r.height()/2.2);
                    QRadialGradient innerHighlight(r.center(), r.width()/2, r.center());
                    innerHighlight.setColorAt(0.0, QColor(255, 255, 255, 0));
                    innerHighlight.setColorAt(1.0, QColor(255, 255, 255, 55));
                    painter->setBrush(innerHighlight);
                    painter->setPen(Qt::NoPen);
                    painter->drawEllipse(highlightRectb);
                    //  painter->drawEllipse(QRectF(1, 1, 16, 16));
                    if (backgroundColor.isValid())
                    {
                        QRectF r(0,0, 18, 18);

                        // --- Degradado principal (radial invertido) ---
                        QRadialGradient base(r.center(), r.width()/2, QPointF(r.center().x(), r.bottom()));
                        base.setColorAt(0.0, baseColor.lighter(110));   // parte baja brillante
                        base.setColorAt(0.6, baseColor);
                        base.setColorAt(1.0, baseColor.darker(110));    // borde oscuro
                        painter->setBrush(base);
                        painter->setPen(baseColor.darker(140));
                        painter->drawEllipse(r);

                        // --- Highlight superior ovalado (reflejo Aqua) ---
                        QRectF highlightRect(r.left()+4, r.top()+1, r.width()-8, r.height()/2.5);
                        QLinearGradient gloss(highlightRect.topLeft(), highlightRect.bottomLeft());
                        gloss.setColorAt(0.0, QColor(255,255,255,180));
                        gloss.setColorAt(1.0, QColor(255,255,255,0));
                        painter->setBrush(gloss);
                        painter->setPen(Qt::NoPen);
                        painter->drawEllipse(highlightRect);

                        // --- Bisel interior claro ---
                        QRectF highlightRectb(r.left()+2, r.top()+r.height()/1.9, r.width()-4, r.height()/2.2);
                        QRadialGradient innerHighlight(r.center(), r.width()/2, r.center());
                        innerHighlight.setColorAt(0.0, QColor(255, 255, 255, 0));
                        innerHighlight.setColorAt(1.0, QColor(255, 255, 255, 55));
                        painter->setBrush(innerHighlight);
                        painter->setPen(Qt::NoPen);
                        painter->drawEllipse(highlightRectb);
                    }
                    if (isPressed() || isHovered()) {
                        /*         if ((isPressed()) && backgroundColor.isValid())
                         { * *
                         painter->setPen(Qt::NoPen);
                         painter->setBrush(backgroundColor);
                         painter->drawEllipse(QRectF(0, 0, 18, 18));
                    } */
                        painter->setPen(pen);
                        painter->setBrush(Qt::NoBrush);

                        QPainterPath path;
                        path.moveTo(5, 6);
                        path.arcTo(QRectF(5, 3.5, 8, 5), 180, -180);
                        path.cubicTo(QPointF(12.5, 9.5), QPointF(9, 7.5), QPointF(9, 11.5));
                        painter->drawPath(path);

                        painter->drawPoint(9, 15);
                    }
                    break;
                }

                default: break;

            }

        }

    }

    //__________________________________________________________________
    void Button::drawIconSunken( QPainter *painter ) const
    {
        painter->setRenderHints(QPainter::Antialiasing);

        /*
         *   scale painter so that its window matches QRect(-1, -1, 20, 20)
         *   this makes all further rendering and scaling simpler
         *   all further rendering is performed inside QRect(0, 0, 18, 18)
         */
        const QRectF rect = geometry().marginsRemoved(m_padding);
        painter->translate(rect.topLeft());

        const qreal width(rect.width());
        painter->scale(width/20, width/20);
        painter->translate(1, 1);

        // render background
        const QColor backgroundColor(this->backgroundColor());

        auto d = qobject_cast<Decoration*>(decoration());
        bool isInactive(d && !d->window()->isActive()
        && !isHovered() && !isPressed()
        && m_animation->state() != QAbstractAnimation::Running);
        QColor inactiveCol(Qt::gray);
        if (isInactive)
        {
            int gray = qGray(d->titleBarColor().rgb());
            if (gray <= 200) {
                gray += 55;
                gray = qMax(gray, 115);
            }
            else gray -= 45;
            inactiveCol = QColor(gray, gray, gray);
        }

        QColor symbolColor;
        symbolColor = QColor(34, 45, 50);
        // render mark
        const QColor foregroundColor(this->foregroundColor(inactiveCol));
        if (foregroundColor.isValid())
        {

            // setup painter
            QPen pen(symbolColor);
            pen.setWidthF(qMax(2.1 * 21 / width, pen.widthF()));

            switch (type())
            {

                case DecorationButtonType::Close:
                {

                    QColor baseColor;

                    if ( !isInactive )
                        baseColor = QColor(255, 92, 87);
                    else
                        baseColor = inactiveCol;

                    QRectF r(0,0, 18, 18);

                    // === Paso 1: fondo liso ===
                    painter->setBrush(baseColor);
                    painter->setPen(Qt::NoPen);
                    painter->drawEllipse(r);

                    // ===== 2) Sombra interior radial =====
                    // Usamos QRadialGradient pero movemos el foco hacia arriba
                    QRadialGradient radial(
                        r.center().x(),           // centro del degradado
                                           r.center().y() + r.height()*0.15,  // foco desplazado hacia arriba
                                           r.width() / 2.0           // radio
                    );
                    radial.setColorAt(0.0, QColor(0, 0, 0, 0));   // se desvanece hacia bordes
                    radial.setColorAt(0.4, QColor(0, 0, 0, 10));
                    radial.setColorAt(0.8, QColor(0, 0, 0, 70));
                    radial.setColorAt(0.95, QColor(0, 0, 0, 120));
                    radial.setColorAt(1.0, QColor(0, 0, 0, 120)); // más oscuro en el foco (arriba)
                    painter->setBrush(radial);
                    painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
                    painter->drawEllipse(r);

                    // === Paso 3: borde sutil ===
                    QPen border(QColor(0,0,0,100), 1);
                    painter->setPen(border);
                    painter->setBrush(Qt::NoBrush);
                    painter->drawEllipse(r);

                    if (backgroundColor.isValid())
                    {
                        QRectF r(0,0, 18, 18);

                        // === Paso 1: fondo liso ===
                        painter->setBrush(baseColor);
                        painter->setPen(Qt::NoPen);
                        painter->drawEllipse(r);

                        // === Paso 2: sombra interior ===
                        // Creamos un degradado vertical que simule la luz entrando por abajo
                        QLinearGradient shadowGrad(r.topLeft(), r.bottomLeft());
                        shadowGrad.setColorAt(0.0, QColor(0, 0, 0, 70));  // sombra fuerte arriba
                        shadowGrad.setColorAt(0.5, QColor(0, 0, 0, 20));
                        shadowGrad.setColorAt(1.0, QColor(0, 0, 0, 0));   // sin sombra abajo

                        // Usamos composición para "restar luz" (sombra interior)
                        painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
                        painter->setBrush(shadowGrad);
                        painter->drawEllipse(r);

                        // === Paso 3: borde sutil ===
                        QPen border(QColor(0,0,0,100), 1);
                        painter->setPen(border);
                        painter->setBrush(Qt::NoBrush);
                        painter->drawEllipse(r);

                    }
                    if (isHovered()) {
                        painter->setPen(pen);
                        painter->setBrush(symbolColor);

                        painter->drawLine( QPointF( 6, 6 ), QPointF( 12, 12 ) );
                        painter->drawLine( QPointF( 6, 12 ), QPointF( 12, 6 ) );
                    }
                    break;
                }

                case DecorationButtonType::Maximize:
                {
                    QColor baseColor;

                    if ( !isInactive )
                        baseColor = QColor(36, 191, 57);
                    else
                        baseColor = inactiveCol;

                    QRectF r(0,0, 18, 18);

                    // === Paso 1: fondo liso ===
                    painter->setBrush(baseColor);
                    painter->setPen(Qt::NoPen);
                    painter->drawEllipse(r);

                    // ===== 2) Sombra interior radial =====
                    // Usamos QRadialGradient pero movemos el foco hacia arriba
                    QRadialGradient radial(
                        r.center().x(),           // centro del degradado
                                           r.center().y() + r.height()*0.15,  // foco desplazado hacia arriba
                                           r.width() / 2.0           // radio
                    );
                    radial.setColorAt(0.0, QColor(0, 0, 0, 0));   // se desvanece hacia bordes
                    radial.setColorAt(0.4, QColor(0, 0, 0, 10));
                    radial.setColorAt(0.8, QColor(0, 0, 0, 70));
                    radial.setColorAt(0.95, QColor(0, 0, 0, 120));
                    radial.setColorAt(1.0, QColor(0, 0, 0, 120)); // más oscuro en el foco (arriba)

                    painter->setBrush(radial);
                    painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
                    painter->drawEllipse(r);

                    // === Paso 3: borde sutil ===
                    QPen border(QColor(0,0,0,100), 1);
                    painter->setPen(border);
                    painter->setBrush(Qt::NoBrush);
                    painter->drawEllipse(r);

                    if (backgroundColor.isValid())
                    {
                        QRectF r(0,0, 18, 18);

                        // === Paso 1: fondo liso ===
                        painter->setBrush(baseColor);
                        painter->setPen(Qt::NoPen);
                        painter->drawEllipse(r);

                        // === Paso 2: sombra interior ===
                        // Creamos un degradado vertical que simule la luz entrando por abajo
                        QLinearGradient shadowGrad(r.topLeft(), r.bottomLeft());
                        shadowGrad.setColorAt(0.0, QColor(0, 0, 0, 70));  // sombra fuerte arriba
                        shadowGrad.setColorAt(0.5, QColor(0, 0, 0, 20));
                        shadowGrad.setColorAt(1.0, QColor(0, 0, 0, 0));   // sin sombra abajo

                        // Usamos composición para "restar luz" (sombra interior)
                        painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
                        painter->setBrush(shadowGrad);
                        painter->drawEllipse(r);

                        // === Paso 3: borde sutil ===
                        QPen border(QColor(0,0,0,100), 1);
                        painter->setPen(border);
                        painter->setBrush(Qt::NoBrush);
                        painter->drawEllipse(r);

                    }
                    if (isHovered()) {
                        painter->setPen( Qt::NoPen );

                        // two triangles
                        QPainterPath path1, path2;
                        if( isChecked() )
                        {
                            path1.moveTo(8.5, 9.5);
                            path1.lineTo(2.5, 9.5);
                            path1.lineTo(8.5, 15.5);

                            path2.moveTo(9.5, 8.5);
                            path2.lineTo(15.5, 8.5);
                            path2.lineTo(9.5, 2.5);
                        }
                        else
                        {
                            path1.moveTo(5, 13);
                            path1.lineTo(11, 13);
                            path1.lineTo(5, 7);

                            path2.moveTo(13, 5);
                            path2.lineTo(7, 5);
                            path2.lineTo(13, 11);
                        }

                        painter->fillPath(path1, QBrush(symbolColor));
                        painter->fillPath(path2, QBrush(symbolColor));
                    }
                    break;
                }

                case DecorationButtonType::Minimize:
                {
                    QColor baseColor;

                    if ( !isInactive )
                        baseColor = QColor(243, 176, 43);
                    else
                        baseColor = inactiveCol;

                    QRectF r(0,0, 18, 18);

                    // === Paso 1: fondo liso ===
                    painter->setBrush(baseColor);
                    painter->setPen(Qt::NoPen);
                    painter->drawEllipse(r);

                    // ===== 2) Sombra interior radial =====
                    // Usamos QRadialGradient pero movemos el foco hacia arriba
                    QRadialGradient radial(
                        r.center().x(),           // centro del degradado
                                           r.center().y() + r.height()*0.15,  // foco desplazado hacia arriba
                                           r.width() / 2.0           // radio
                    );
                    radial.setColorAt(0.0, QColor(0, 0, 0, 0));   // se desvanece hacia bordes
                    radial.setColorAt(0.4, QColor(0, 0, 0, 10));
                    radial.setColorAt(0.8, QColor(0, 0, 0, 70));
                    radial.setColorAt(0.95, QColor(0, 0, 0, 120));
                    radial.setColorAt(1.0, QColor(0, 0, 0, 120)); // más oscuro en el foco (arriba)

                    painter->setBrush(radial);
                    painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
                    painter->drawEllipse(r);

                    // === Paso 3: borde sutil ===
                    QPen border(QColor(0,0,0,100), 1);
                    painter->setPen(border);
                    painter->setBrush(Qt::NoBrush);
                    painter->drawEllipse(r);

                    if (backgroundColor.isValid())
                    {
                        QRectF r(0,0, 18, 18);

                        // === Paso 1: fondo liso ===
                        painter->setBrush(baseColor);
                        painter->setPen(Qt::NoPen);
                        painter->drawEllipse(r);

                        // === Paso 2: sombra interior ===
                        // Creamos un degradado vertical que simule la luz entrando por abajo
                        QLinearGradient shadowGrad(r.topLeft(), r.bottomLeft());
                        shadowGrad.setColorAt(0.0, QColor(0, 0, 0, 70));  // sombra fuerte arriba
                        shadowGrad.setColorAt(0.5, QColor(0, 0, 0, 20));
                        shadowGrad.setColorAt(1.0, QColor(0, 0, 0, 0));   // sin sombra abajo

                        // Usamos composición para "restar luz" (sombra interior)
                        painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
                        painter->setBrush(shadowGrad);
                        painter->drawEllipse(r);

                        // === Paso 3: borde sutil ===
                        QPen border(QColor(0,0,0,100), 1);
                        painter->setPen(border);
                        painter->setBrush(Qt::NoBrush);
                        painter->drawEllipse(r);

                    }
                    if (isHovered()) {
                        pen.setWidthF(1.2*qMax((qreal)1.0, 20/width));
                        painter->setPen(pen);
                        painter->setBrush(symbolColor);
                        painter->drawLine( QPointF( 5, 9 ), QPointF( 13, 9 ) );
                    }
                    break;
                }

                case DecorationButtonType::OnAllDesktops:
                {
                    QColor baseColor;

                    if ( !isInactive )
                        baseColor = QColor(132, 165, 202);
                    else
                        baseColor = inactiveCol;

                    QRectF r(0,0, 18, 18);

                    // === Paso 1: fondo liso ===
                    painter->setBrush(baseColor);
                    painter->setPen(Qt::NoPen);
                    painter->drawEllipse(r);

                    // ===== 2) Sombra interior radial =====
                    // Usamos QRadialGradient pero movemos el foco hacia arriba
                    QRadialGradient radial(
                        r.center().x(),           // centro del degradado
                                           r.center().y() + r.height()*0.10,  // foco desplazado hacia arriba
                                           r.width() / 2.0           // radio
                    );
                    radial.setColorAt(0.0, QColor(0, 0, 0, 0));   // se desvanece hacia bordes
                    radial.setColorAt(0.4, QColor(0, 0, 0, 10));
                    radial.setColorAt(0.8, QColor(0, 0, 0, 70));
                    radial.setColorAt(0.95, QColor(0, 0, 0, 120));
                    radial.setColorAt(1.0, QColor(0, 0, 0, 120)); // más oscuro en el foco (arriba)

                    painter->setBrush(radial);
                    painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
                    painter->drawEllipse(r);

                    // === Paso 3: borde sutil ===
                    QPen border(QColor(0,0,0,100), 1);
                    painter->setPen(border);
                    painter->setBrush(Qt::NoBrush);
                    painter->drawEllipse(r);

                    if (backgroundColor.isValid())
                    {
                        QRectF r(0,0, 18, 18);

                        // === Paso 1: fondo liso ===
                        painter->setBrush(baseColor);
                        painter->setPen(Qt::NoPen);
                        painter->drawEllipse(r);

                        // === Paso 2: sombra interior ===
                        // Creamos un degradado vertical que simule la luz entrando por abajo
                        QLinearGradient shadowGrad(r.topLeft(), r.bottomLeft());
                        shadowGrad.setColorAt(0.0, QColor(0, 0, 0, 70));  // sombra fuerte arriba
                        shadowGrad.setColorAt(0.5, QColor(0, 0, 0, 20));
                        shadowGrad.setColorAt(1.0, QColor(0, 0, 0, 0));   // sin sombra abajo

                        // Usamos composición para "restar luz" (sombra interior)
                        painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
                        painter->setBrush(shadowGrad);
                        painter->drawEllipse(r);

                        // === Paso 3: borde sutil ===
                        QPen border(QColor(0,0,0,100), 1);
                        painter->setPen(border);
                        painter->setBrush(Qt::NoBrush);
                        painter->drawEllipse(r);
                    }
                    if (isPressed() || isHovered() || isChecked()) {
                        /*         if ((isPressed()) && backgroundColor.isValid())
                         *                    { * * *
                         *                    painter->setPen(Qt::NoPen);
                         *                    painter->setBrush(backgroundColor);
                         *                    painter->drawEllipse(QRectF(0, 0, 18, 18));
                    } */
                        painter->setPen(pen);
                        painter->setBrush(Qt::NoBrush);
                        painter->drawEllipse(QRectF(6, 6, 6, 6));
                    }
                    break;
                }

                case DecorationButtonType::Shade:
                {
                    QColor baseColor;

                    if ( !isInactive )
                        baseColor = QColor(132, 165, 202);
                    else
                        baseColor = inactiveCol;

                    QRectF r(0,0, 18, 18);

                    // === Paso 1: fondo liso ===
                    painter->setBrush(baseColor);
                    painter->setPen(Qt::NoPen);
                    painter->drawEllipse(r);

                    // ===== 2) Sombra interior radial =====
                    // Usamos QRadialGradient pero movemos el foco hacia arriba
                    QRadialGradient radial(
                        r.center().x(),           // centro del degradado
                                           r.center().y() + r.height()*0.10,  // foco desplazado hacia arriba
                                           r.width() / 2.0           // radio
                    );
                    radial.setColorAt(0.0, QColor(0, 0, 0, 0));   // se desvanece hacia bordes
                    radial.setColorAt(0.4, QColor(0, 0, 0, 10));
                    radial.setColorAt(0.8, QColor(0, 0, 0, 70));
                    radial.setColorAt(0.95, QColor(0, 0, 0, 120));
                    radial.setColorAt(1.0, QColor(0, 0, 0, 120)); // más oscuro en el foco (arriba)

                    painter->setBrush(radial);
                    painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
                    painter->drawEllipse(r);

                    // === Paso 3: borde sutil ===
                    QPen border(QColor(0,0,0,100), 1);
                    painter->setPen(border);
                    painter->setBrush(Qt::NoBrush);
                    painter->drawEllipse(r);

                    if (backgroundColor.isValid())
                    {
                        QRectF r(0,0, 18, 18);

                        // === Paso 1: fondo liso ===
                        painter->setBrush(baseColor);
                        painter->setPen(Qt::NoPen);
                        painter->drawEllipse(r);

                        // === Paso 2: sombra interior ===
                        // Creamos un degradado vertical que simule la luz entrando por abajo
                        QLinearGradient shadowGrad(r.topLeft(), r.bottomLeft());
                        shadowGrad.setColorAt(0.0, QColor(0, 0, 0, 70));  // sombra fuerte arriba
                        shadowGrad.setColorAt(0.5, QColor(0, 0, 0, 20));
                        shadowGrad.setColorAt(1.0, QColor(0, 0, 0, 0));   // sin sombra abajo

                        // Usamos composición para "restar luz" (sombra interior)
                        painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
                        painter->setBrush(shadowGrad);
                        painter->drawEllipse(r);

                        // === Paso 3: borde sutil ===
                        QPen border(QColor(0,0,0,100), 1);
                        painter->setPen(border);
                        painter->setBrush(Qt::NoBrush);
                        painter->drawEllipse(r);
                    }
                    if (isPressed() || isHovered() || isChecked()) {
                        /*         if ((isPressed()) && backgroundColor.isValid())
                         *                    { * *
                         *                    painter->setPen(Qt::NoPen);
                         *                    painter->setBrush(backgroundColor);
                         *                    painter->drawEllipse(QRectF(0, 0, 18, 18));
                    } */
                        painter->setPen(pen);
                        painter->setBrush(Qt::NoBrush);
                        painter->drawPolyline(QPolygonF()
                        << QPointF(5, 13)
                        << QPointF(9, 9)
                        << QPointF(13, 13));
                    }
                    break;

                }

                case DecorationButtonType::KeepBelow:
                {
                    QColor baseColor;

                    if ( !isInactive )
                        baseColor = QColor(132, 165, 202);
                    else
                        baseColor = inactiveCol;

                    QRectF r(0,0, 18, 18);

                    // === Paso 1: fondo liso ===
                    painter->setBrush(baseColor);
                    painter->setPen(Qt::NoPen);
                    painter->drawEllipse(r);

                    // ===== 2) Sombra interior radial =====
                    // Usamos QRadialGradient pero movemos el foco hacia arriba
                    QRadialGradient radial(
                        r.center().x(),           // centro del degradado
                                           r.center().y() + r.height()*0.10,  // foco desplazado hacia arriba
                                           r.width() / 2.0           // radio
                    );
                    radial.setColorAt(0.0, QColor(0, 0, 0, 0));   // se desvanece hacia bordes
                    radial.setColorAt(0.4, QColor(0, 0, 0, 10));
                    radial.setColorAt(0.8, QColor(0, 0, 0, 70));
                    radial.setColorAt(0.95, QColor(0, 0, 0, 120));
                    radial.setColorAt(1.0, QColor(0, 0, 0, 120)); // más oscuro en el foco (arriba)

                    painter->setBrush(radial);
                    painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
                    painter->drawEllipse(r);

                    // === Paso 3: borde sutil ===
                    QPen border(QColor(0,0,0,100), 1);
                    painter->setPen(border);
                    painter->setBrush(Qt::NoBrush);
                    painter->drawEllipse(r);

                    if (backgroundColor.isValid())
                    {
                        QRectF r(0,0, 18, 18);

                        // === Paso 1: fondo liso ===
                        painter->setBrush(baseColor);
                        painter->setPen(Qt::NoPen);
                        painter->drawEllipse(r);

                        // === Paso 2: sombra interior ===
                        // Creamos un degradado vertical que simule la luz entrando por abajo
                        QLinearGradient shadowGrad(r.topLeft(), r.bottomLeft());
                        shadowGrad.setColorAt(0.0, QColor(0, 0, 0, 70));  // sombra fuerte arriba
                        shadowGrad.setColorAt(0.5, QColor(0, 0, 0, 20));
                        shadowGrad.setColorAt(1.0, QColor(0, 0, 0, 0));   // sin sombra abajo

                        // Usamos composición para "restar luz" (sombra interior)
                        painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
                        painter->setBrush(shadowGrad);
                        painter->drawEllipse(r);

                        // === Paso 3: borde sutil ===
                        QPen border(QColor(0,0,0,100), 1);
                        painter->setPen(border);
                        painter->setBrush(Qt::NoBrush);
                        painter->drawEllipse(r);
                    }
                    if (isPressed() || isHovered() || isChecked()) {
                        /*         if ((isPressed()) && backgroundColor.isValid())
                         *                    { *
                         *                    painter->setPen(Qt::NoPen);
                         *                    painter->setBrush(backgroundColor);
                         *                    painter->drawEllipse(QRectF(0, 0, 18, 18));
                    } */
                        painter->setPen(pen);
                        painter->setBrush(Qt::NoBrush);
                        painter->drawPolyline(QPolygonF()
                        << QPointF(6, 6)
                        << QPointF(9, 9)
                        << QPointF(12, 6));

                        painter->drawPolyline(QPolygonF()
                        << QPointF(6, 10)
                        << QPointF(9, 13)
                        << QPointF(12, 10));
                    }
                    break;

                }

                case DecorationButtonType::KeepAbove:
                {
                    QColor baseColor;

                    if ( !isInactive )
                        baseColor = QColor(132, 165, 202);
                    else
                        baseColor = inactiveCol;

                    QRectF r(0,0, 18, 18);

                    // === Paso 1: fondo liso ===
                    painter->setBrush(baseColor);
                    painter->setPen(Qt::NoPen);
                    painter->drawEllipse(r);

                    // ===== 2) Sombra interior radial =====
                    // Usamos QRadialGradient pero movemos el foco hacia arriba
                    QRadialGradient radial(
                        r.center().x(),           // centro del degradado
                                           r.center().y() + r.height()*0.10,  // foco desplazado hacia arriba
                                           r.width() / 2.0           // radio
                    );
                    radial.setColorAt(0.0, QColor(0, 0, 0, 0));   // se desvanece hacia bordes
                    radial.setColorAt(0.4, QColor(0, 0, 0, 10));
                    radial.setColorAt(0.8, QColor(0, 0, 0, 70));
                    radial.setColorAt(0.95, QColor(0, 0, 0, 120));
                    radial.setColorAt(1.0, QColor(0, 0, 0, 120)); // más oscuro en el foco (arriba)

                    painter->setBrush(radial);
                    painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
                    painter->drawEllipse(r);

                    // === Paso 3: borde sutil ===
                    QPen border(QColor(0,0,0,100), 1);
                    painter->setPen(border);
                    painter->setBrush(Qt::NoBrush);
                    painter->drawEllipse(r);

                    if (backgroundColor.isValid())
                    {
                        QRectF r(0,0, 18, 18);

                        // === Paso 1: fondo liso ===
                        painter->setBrush(baseColor);
                        painter->setPen(Qt::NoPen);
                        painter->drawEllipse(r);

                        // === Paso 2: sombra interior ===
                        // Creamos un degradado vertical que simule la luz entrando por abajo
                        QLinearGradient shadowGrad(r.topLeft(), r.bottomLeft());
                        shadowGrad.setColorAt(0.0, QColor(0, 0, 0, 70));  // sombra fuerte arriba
                        shadowGrad.setColorAt(0.5, QColor(0, 0, 0, 20));
                        shadowGrad.setColorAt(1.0, QColor(0, 0, 0, 0));   // sin sombra abajo

                        // Usamos composición para "restar luz" (sombra interior)
                        painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
                        painter->setBrush(shadowGrad);
                        painter->drawEllipse(r);

                        // === Paso 3: borde sutil ===
                        QPen border(QColor(0,0,0,100), 1);
                        painter->setPen(border);
                        painter->setBrush(Qt::NoBrush);
                        painter->drawEllipse(r);
                    }
                    if (isPressed() || isHovered() || isChecked()) {
                        /*         if ((isPressed()) && backgroundColor.isValid())
                         *                   {
                         *                       painter->setPen(Qt::NoPen);
                         *                       painter->setBrush(backgroundColor);
                         *                       painter->drawEllipse(QRectF(0, 0, 18, 18));
                    } */
                        painter->setPen(pen);
                        painter->setBrush(Qt::NoBrush);
                        painter->drawPolyline(QPolygonF()
                        << QPointF(6, 8)
                        << QPointF(9, 5)
                        << QPointF(12, 8));

                        painter->drawPolyline(QPolygonF()
                        << QPointF(6, 12)
                        << QPointF(9, 9)
                        << QPointF(12, 12));
                    }
                    break;
                }


                case DecorationButtonType::ApplicationMenu:
                {
                    QColor baseColor;

                    if ( !isInactive )
                        baseColor = QColor(230, 129, 67);
                    else
                        baseColor = inactiveCol;

                    QRectF r(0,0, 18, 18);

                    // === Paso 1: fondo liso ===
                    painter->setBrush(baseColor);
                    painter->setPen(Qt::NoPen);
                    painter->drawEllipse(r);

                    // ===== 2) Sombra interior radial =====
                    // Usamos QRadialGradient pero movemos el foco hacia arriba
                    QRadialGradient radial(
                        r.center().x(),           // centro del degradado
                                           r.center().y() + r.height()*0.10,  // foco desplazado hacia arriba
                                           r.width() / 2.0           // radio
                    );
                    radial.setColorAt(0.0, QColor(0, 0, 0, 0));   // se desvanece hacia bordes
                    radial.setColorAt(0.4, QColor(0, 0, 0, 10));
                    radial.setColorAt(0.8, QColor(0, 0, 0, 70));
                    radial.setColorAt(0.95, QColor(0, 0, 0, 120));
                    radial.setColorAt(1.0, QColor(0, 0, 0, 120)); // más oscuro en el foco (arriba)

                    painter->setBrush(radial);
                    painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
                    painter->drawEllipse(r);

                    // === Paso 3: borde sutil ===
                    QPen border(QColor(0,0,0,100), 1);
                    painter->setPen(border);
                    painter->setBrush(Qt::NoBrush);
                    painter->drawEllipse(r);

                    if (backgroundColor.isValid())
                    {
                        QRectF r(0,0, 18, 18);

                        // === Paso 1: fondo liso ===
                        painter->setBrush(baseColor);
                        painter->setPen(Qt::NoPen);
                        painter->drawEllipse(r);

                        // === Paso 2: sombra interior ===
                        // Creamos un degradado vertical que simule la luz entrando por abajo
                        QLinearGradient shadowGrad(r.topLeft(), r.bottomLeft());
                        shadowGrad.setColorAt(0.0, QColor(0, 0, 0, 70));  // sombra fuerte arriba
                        shadowGrad.setColorAt(0.5, QColor(0, 0, 0, 20));
                        shadowGrad.setColorAt(1.0, QColor(0, 0, 0, 0));   // sin sombra abajo

                        // Usamos composición para "restar luz" (sombra interior)
                        painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
                        painter->setBrush(shadowGrad);
                        painter->drawEllipse(r);

                        // === Paso 3: borde sutil ===
                        QPen border(QColor(0,0,0,100), 1);
                        painter->setPen(border);
                        painter->setBrush(Qt::NoBrush);
                        painter->drawEllipse(r);
                    }
                    if (isPressed() || isHovered()) {
                        /*         if ((isPressed()) && backgroundColor.isValid())
                         *                    { *
                         *                    painter->setPen(Qt::NoPen);
                         *                    painter->setBrush(backgroundColor);
                         *                    painter->drawEllipse(QRectF(0, 0, 18, 18));
                    } */
                        painter->setPen(pen);
                        painter->setBrush(Qt::NoBrush);
                        painter->drawLine(QPointF(4.5, 6), QPointF(13.5, 6));
                        painter->drawLine(QPointF(4.5, 9), QPointF(13.5, 9));
                        painter->drawLine(QPointF(4.5, 12), QPointF(13.5, 12));
                    }
                    break;
                }

                case DecorationButtonType::ContextHelp:
                {
                    QColor baseColor;

                    if ( !isInactive )
                        baseColor = QColor(230, 129, 67);
                    else
                        baseColor = inactiveCol;

                    QRectF r(0,0, 18, 18);

                    // === Paso 1: fondo liso ===
                    painter->setBrush(baseColor);
                    painter->setPen(Qt::NoPen);
                    painter->drawEllipse(r);

                    // ===== 2) Sombra interior radial =====
                    // Usamos QRadialGradient pero movemos el foco hacia arriba
                    QRadialGradient radial(
                        r.center().x(),           // centro del degradado
                                           r.center().y() + r.height()*0.10,  // foco desplazado hacia arriba
                                           r.width() / 2.0           // radio
                    );
                    radial.setColorAt(0.0, QColor(0, 0, 0, 0));   // se desvanece hacia bordes
                    radial.setColorAt(0.4, QColor(0, 0, 0, 10));
                    radial.setColorAt(0.8, QColor(0, 0, 0, 70));
                    radial.setColorAt(0.95, QColor(0, 0, 0, 120));
                    radial.setColorAt(1.0, QColor(0, 0, 0, 120)); // más oscuro en el foco (arriba)

                    painter->setBrush(radial);
                    painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
                    painter->drawEllipse(r);

                    // === Paso 3: borde sutil ===
                    QPen border(QColor(0,0,0,100), 1);
                    painter->setPen(border);
                    painter->setBrush(Qt::NoBrush);
                    painter->drawEllipse(r);

                    if (backgroundColor.isValid())
                    {
                        QRectF r(0,0, 18, 18);

                        // === Paso 1: fondo liso ===
                        painter->setBrush(baseColor);
                        painter->setPen(Qt::NoPen);
                        painter->drawEllipse(r);

                        // === Paso 2: sombra interior ===
                        // Creamos un degradado vertical que simule la luz entrando por abajo
                        QLinearGradient shadowGrad(r.topLeft(), r.bottomLeft());
                        shadowGrad.setColorAt(0.0, QColor(0, 0, 0, 70));  // sombra fuerte arriba
                        shadowGrad.setColorAt(0.5, QColor(0, 0, 0, 20));
                        shadowGrad.setColorAt(1.0, QColor(0, 0, 0, 0));   // sin sombra abajo

                        // Usamos composición para "restar luz" (sombra interior)
                        painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
                        painter->setBrush(shadowGrad);
                        painter->drawEllipse(r);

                        // === Paso 3: borde sutil ===
                        QPen border(QColor(0,0,0,100), 1);
                        painter->setPen(border);
                        painter->setBrush(Qt::NoBrush);
                        painter->drawEllipse(r);
                    }
                    if (isPressed() || isHovered()) {
                        /*         if ((isPressed()) && backgroundColor.isValid())
                         *                    { * *
                         *                    painter->setPen(Qt::NoPen);
                         *                    painter->setBrush(backgroundColor);
                         *                    painter->drawEllipse(QRectF(0, 0, 18, 18));
                    } */
                        painter->setPen(pen);
                        painter->setBrush(Qt::NoBrush);

                        QPainterPath path;
                        path.moveTo(5, 6);
                        path.arcTo(QRectF(5, 3.5, 8, 5), 180, -180);
                        path.cubicTo(QPointF(12.5, 9.5), QPointF(9, 7.5), QPointF(9, 11.5));
                        painter->drawPath(path);

                        painter->drawPoint(9, 15);
                    }
                    break;
                }

                default: break;

            }

        }

    }

    //__________________________________________________________________
    void Button::drawIconMacSymbols(QPainter *painter) const
    {

        painter->setRenderHints(QPainter::Antialiasing);

        /*
         *   scale painter so that its window matches QRect(-1, -1, 20, 20)
         *   this makes all further rendering and scaling simpler
         *   all further rendering is performed inside QRect(0, 0, 18, 18)
         */
        const QRectF rect = geometry().marginsRemoved(m_padding);
        painter->translate(rect.topLeft());

        const qreal width(rect.width());
        painter->scale(width/20, width/20);
        painter->translate(1, 1);

        // render background
        const QColor backgroundColor(this->backgroundColor());

        auto d = qobject_cast<Decoration*>(decoration());
        bool isInactive(d && !d->window()->isActive()
        && !isHovered() && !isPressed()
        && m_animation->state() != QAbstractAnimation::Running);
        QColor inactiveCol(Qt::gray);
        if (isInactive)
        {
            int gray = qGray(d->titleBarColor().rgb());
            if (gray <= 200) {
                gray += 55;
                gray = qMax(gray, 115);
            }
            else gray -= 45;
            inactiveCol = QColor(gray, gray, gray);
        }

        QColor symbolColor;
        symbolColor = QColor(34, 45, 50);
        // render mark
        const QColor foregroundColor(this->foregroundColor(inactiveCol));
        if (foregroundColor.isValid())
        {

            // setup painter
            QPen pen(symbolColor);
            pen.setWidthF(qMax(2.1 * 21 / width, pen.widthF()));

            switch (type())
            {

                case DecorationButtonType::Close:
                {
                        QLinearGradient grad(QPointF(9, 2), QPointF(9, 16));
                        grad.setColorAt(0, isInactive ? inactiveCol
                        : QColor(255, 92, 87));
                        grad.setColorAt(1, isInactive ? inactiveCol
                        : QColor(233, 84, 79));
                        QColor baseColor;

                        if ( !isInactive )
                            baseColor = QColor(255, 92, 87);
                        else
                            baseColor = inactiveCol;

                        painter->setBrush(QBrush(grad));
                        painter->setPen(baseColor.darker(140));
                        painter->drawEllipse(QRectF(1, 1, 16, 16));
                        if (backgroundColor.isValid())
                        {
                            painter->setPen(baseColor.darker(140));
                            painter->setBrush(backgroundColor);
                            qreal r = static_cast<qreal>(7)
                            + (isPressed() ? 0.0
                            : static_cast<qreal>(2) * m_animation->currentValue().toReal());
                            QPointF c(static_cast<qreal>(9), static_cast<qreal>(9));
                            painter->drawEllipse(c, r, r);
                        }
                        if (isHovered()) {
                            painter->setPen(pen);
                            painter->setBrush(symbolColor);

                            painter->drawLine( QPointF( 6, 6 ), QPointF( 12, 12 ) );
                            painter->drawLine( QPointF( 6, 12 ), QPointF( 12, 6 ) );
                        }
                    break;
                }

                case DecorationButtonType::Maximize:
                {
                        QLinearGradient grad(QPointF(9, 2), QPointF(9, 16));
                        grad.setColorAt(0, isChecked() ? isInactive ? inactiveCol
                        : QColor(67, 198, 176)
                        : isInactive ? inactiveCol
                        : QColor(40, 211, 63));
                        grad.setColorAt(1, isChecked() ? isInactive ? inactiveCol
                        : QColor(60, 178, 159)
                        : isInactive ? inactiveCol
                        : QColor(36, 191, 57));

                        QColor baseColor;

                        if ( !isInactive )
                            baseColor = QColor(36, 191, 57);
                        else
                             baseColor = inactiveCol;

                        painter->setBrush(QBrush(grad));
                        painter->setPen(baseColor.darker(140));
                        painter->drawEllipse(QRectF(1, 1, 16, 16));
                        if (backgroundColor.isValid())
                        {
                            painter->setPen(baseColor.darker(140));
                            painter->setBrush(backgroundColor);
                            qreal r = static_cast<qreal>(7)
                            + (isPressed() ? 0.0
                            : static_cast<qreal>(2) * m_animation->currentValue().toReal());
                            QPointF c(static_cast<qreal>(9), static_cast<qreal>(9));
                            painter->drawEllipse(c, r, r);
                        }
                        if (isHovered()) {
                            painter->setPen( Qt::NoPen );

                            // two triangles
                            QPainterPath path1, path2;
                            if( isChecked() )
                            {
                                path1.moveTo(8.5, 9.5);
                                path1.lineTo(2.5, 9.5);
                                path1.lineTo(8.5, 15.5);

                                path2.moveTo(9.5, 8.5);
                                path2.lineTo(15.5, 8.5);
                                path2.lineTo(9.5, 2.5);
                            }
                            else
                            {
                                path1.moveTo(5, 13);
                                path1.lineTo(11, 13);
                                path1.lineTo(5, 7);

                                path2.moveTo(13, 5);
                                path2.lineTo(7, 5);
                                path2.lineTo(13, 11);
                            }

                            painter->fillPath(path1, QBrush(symbolColor));
                            painter->fillPath(path2, QBrush(symbolColor));
                        }
                    break;
                }

                case DecorationButtonType::Minimize:
                {
                        QLinearGradient grad(QPointF(9, 2), QPointF(9, 16));
                        grad.setColorAt(0, isInactive ? inactiveCol
                        : QColor(243, 176, 43));
                        grad.setColorAt(1, isInactive ? inactiveCol
                        : QColor(223, 162, 39));
                        QColor baseColor;

                        if ( !isInactive )
                            baseColor = QColor(243, 176, 43);
                        else
                            baseColor = inactiveCol;

                        painter->setBrush(QBrush(grad));
                        painter->setPen(baseColor.darker(140));
                        painter->drawEllipse(QRectF(1, 1, 16, 16));
                        if (backgroundColor.isValid())
                        {
                            painter->setPen(baseColor.darker(140));
                            painter->setBrush(backgroundColor);
                            qreal r = static_cast<qreal>(7)
                            + (isPressed() ? 0.0
                            : static_cast<qreal>(2) * m_animation->currentValue().toReal());
                            QPointF c(static_cast<qreal>(9), static_cast<qreal>(9));
                            painter->drawEllipse(c, r, r);
                        }
                        if (isHovered()) {
                            pen.setWidthF(1.2*qMax((qreal)1.0, 20/width));
                            painter->setPen(pen);
                            painter->setBrush(symbolColor);
                            painter->drawLine( QPointF( 5, 9 ), QPointF( 13, 9 ) );
                        }
                    break;
                }

                case DecorationButtonType::OnAllDesktops:
                {
                    QColor baseColor;

                    if ( !isInactive )
                        baseColor = QColor(103, 149, 210);
                    else
                        baseColor = inactiveCol;

                    if (!isPressed()) {
                        QLinearGradient grad(QPointF(9, 2), QPointF(9, 16));
                        grad.setColorAt(0, isInactive ? inactiveCol
                        : QColor(103, 149, 210));
                        grad.setColorAt(1, isInactive ? inactiveCol
                        : QColor(93, 135, 190));

                        painter->setBrush(QBrush(grad));
                        painter->setPen(baseColor.darker(140));
                        if (isChecked())
                            painter->drawEllipse(QRectF(0, 0, 18, 18));
                        else {
                            painter->drawEllipse(QRectF(1, 1, 16, 16));
                            if (backgroundColor.isValid())
                            {
                                painter->setPen(baseColor.darker(140));
                                painter->setBrush(backgroundColor);
                                qreal r = static_cast<qreal>(7)
                                + static_cast<qreal>(2) * m_animation->currentValue().toReal();
                                QPointF c(static_cast<qreal>(9), static_cast<qreal>(9));
                                painter->drawEllipse(c, r, r);
                            }
                        }
                    }
                    if (isPressed() || isHovered() || isChecked()) {
                        if ((isPressed()) && backgroundColor.isValid())
                        {
                            painter->setPen(baseColor.darker(140));
                            painter->setBrush(backgroundColor);
                            painter->drawEllipse(QRectF(0, 0, 18, 18));
                        }
                        painter->setPen(pen);
                        painter->setBrush(Qt::NoBrush);
                        painter->drawEllipse(QRectF(6, 6, 6, 6));

                    }
                    break;
                }

                case DecorationButtonType::Shade:
                {
                    QColor baseColor;

                    if ( !isInactive )
                        baseColor = QColor(103, 149, 210);
                    else
                        baseColor = inactiveCol;

                    if (!isPressed()) {
                        QLinearGradient grad(QPointF(9, 2), QPointF(9, 16));
                        grad.setColorAt(0, isInactive ? inactiveCol
                        : QColor(103, 149, 210));
                        grad.setColorAt(1, isInactive ? inactiveCol
                        : QColor(93, 135, 190));

                        painter->setBrush(QBrush(grad));
                        painter->setPen(baseColor.darker(140));
                        if (isChecked())
                            painter->drawEllipse(QRectF(0, 0, 18, 18));
                        else {
                            painter->drawEllipse(QRectF(1, 1, 16, 16));
                            if (backgroundColor.isValid())
                            {
                                painter->setPen(baseColor.darker(140));
                                painter->setBrush(backgroundColor);
                                qreal r = static_cast<qreal>(7)
                                + static_cast<qreal>(2) * m_animation->currentValue().toReal();
                                QPointF c(static_cast<qreal>(9), static_cast<qreal>(9));
                                painter->drawEllipse(c, r, r);
                            }
                        }
                    }
                    if (isPressed() || isHovered() || isChecked()) {
                        if ((isPressed()) && backgroundColor.isValid())
                        {
                            painter->setPen(baseColor.darker(140));
                            painter->setBrush(backgroundColor);
                            painter->drawEllipse(QRectF(0, 0, 18, 18));
                        }
                        painter->setPen(pen);
                        painter->setBrush(Qt::NoBrush);

                        painter->drawLine(5, 6, 13, 6);
                        if (isChecked()) {
                            painter->drawPolyline(QPolygonF()
                            << QPointF(5, 9)
                            << QPointF(9, 13)
                            << QPointF(13, 9));

                        }
                        else {
                            painter->drawPolyline(QPolygonF()
                            << QPointF(5, 13)
                            << QPointF(9, 9)
                            << QPointF(13, 13));
                        }
                    }

                    break;

                }

                case DecorationButtonType::KeepBelow:
                {
                    QColor baseColor;

                    if ( !isInactive )
                        baseColor = QColor(103, 149, 210);
                    else
                        baseColor = inactiveCol;

                    if (!isPressed()) {
                        QLinearGradient grad(QPointF(9, 2), QPointF(9, 16));
                        grad.setColorAt(0, isInactive ? inactiveCol
                        : QColor(103, 149, 210));
                        grad.setColorAt(1, isInactive ? inactiveCol
                        : QColor(93, 135, 190));

                        painter->setBrush(QBrush(grad));
                        painter->setPen(baseColor.darker(140));
                        if (isChecked())
                            painter->drawEllipse(QRectF(0, 0, 18, 18));
                        else {
                            painter->drawEllipse(QRectF(1, 1, 16, 16));
                            if (backgroundColor.isValid())
                            {
                                painter->setPen(baseColor.darker(140));
                                painter->setBrush(backgroundColor);
                                qreal r = static_cast<qreal>(7)
                                + static_cast<qreal>(2) * m_animation->currentValue().toReal();
                                QPointF c(static_cast<qreal>(9), static_cast<qreal>(9));
                                painter->drawEllipse(c, r, r);
                            }
                        }
                    }
                    if (isPressed() || isHovered() || isChecked()) {
                        if ((isPressed()) && backgroundColor.isValid())
                        {
                            painter->setPen(baseColor.darker(140));
                            painter->setBrush(backgroundColor);
                            painter->drawEllipse(QRectF(0, 0, 18, 18));
                        }
                        painter->setPen(pen);
                        painter->setBrush(Qt::NoBrush);
                        painter->drawPolyline(QPolygonF()
                        << QPointF(6, 6)
                        << QPointF(9, 9)
                        << QPointF(12, 6));

                        painter->drawPolyline(QPolygonF()
                        << QPointF(6, 10)
                        << QPointF(9, 13)
                        << QPointF(12, 10));
                    }
                    break;

                }

                case DecorationButtonType::KeepAbove:
                {
                    QColor baseColor;

                    if ( !isInactive )
                        baseColor = QColor(103, 149, 210);
                    else
                        baseColor = inactiveCol;

                    if (!isPressed()) {
                        QLinearGradient grad(QPointF(9, 2), QPointF(9, 16));
                        grad.setColorAt(0, isInactive ? inactiveCol
                        : QColor(103, 149, 210));
                        grad.setColorAt(1, isInactive ? inactiveCol
                        : QColor(93, 135, 190));

                        painter->setBrush(QBrush(grad));
                        painter->setPen(baseColor.darker(140));
                        if (isChecked())
                            painter->drawEllipse(QRectF(0, 0, 18, 18));
                        else {
                            painter->drawEllipse(QRectF(1, 1, 16, 16));
                            if (backgroundColor.isValid())
                            {
                                painter->setPen(baseColor.darker(140));
                                painter->setBrush(backgroundColor);
                                qreal r = static_cast<qreal>(7)
                                + static_cast<qreal>(2) * m_animation->currentValue().toReal();
                                QPointF c(static_cast<qreal>(9), static_cast<qreal>(9));
                                painter->drawEllipse(c, r, r);
                            }
                        }
                    }
                    if (isPressed() || isHovered() || isChecked()) {
                        if ((isPressed()) && backgroundColor.isValid())
                        {
                            painter->setPen(baseColor.darker(140));
                            painter->setBrush(backgroundColor);
                            painter->drawEllipse(QRectF(0, 0, 18, 18));
                        }
                        painter->setPen(pen);
                        painter->setBrush(Qt::NoBrush);
                            painter->drawPolyline(QPolygonF()
                            << QPointF(6, 8)
                            << QPointF(9, 5)
                            << QPointF(12, 8));

                            painter->drawPolyline(QPolygonF()
                            << QPointF(6, 12)
                            << QPointF(9, 9)
                            << QPointF(12, 12));
                    }
                    break;
                }


                case DecorationButtonType::ApplicationMenu:
                {
                    QColor baseColor;

                    if ( !isInactive )
                        baseColor = QColor(230, 129, 67);
                    else
                        baseColor = inactiveCol;

                    if (!isPressed()) {
                        QLinearGradient grad(QPointF(9, 2), QPointF(9, 16));
                        grad.setColorAt(0, isInactive ? inactiveCol
                        : QColor(230, 129, 67));
                        grad.setColorAt(1, isInactive ? inactiveCol
                        : QColor(210, 118, 61));

                        painter->setBrush(QBrush(grad));
                        painter->setPen(baseColor.darker(140));
                        painter->drawEllipse(QRectF(1, 1, 16, 16));
                        if (backgroundColor.isValid())
                        {
                            painter->setPen(baseColor.darker(140));
                            painter->setBrush(backgroundColor);
                            qreal r = static_cast<qreal>(7)
                            + static_cast<qreal>(2) * m_animation->currentValue().toReal();
                            QPointF c(static_cast<qreal>(9), static_cast<qreal>(9));
                            painter->drawEllipse(c, r, r);
                        }
                    }
                    if (isPressed() || isHovered()) {
                        if ((isPressed()) && backgroundColor.isValid())
                        {
                            painter->setPen(baseColor.darker(140));
                            painter->setBrush(backgroundColor);
                            painter->drawEllipse(QRectF(0, 0, 18, 18));
                        }
                        painter->setPen(pen);
                        painter->setBrush(Qt::NoBrush);
                            painter->drawLine(QPointF(4.5, 6), QPointF(13.5, 6));
                            painter->drawLine(QPointF(4.5, 9), QPointF(13.5, 9));
                            painter->drawLine(QPointF(4.5, 12), QPointF(13.5, 12));
                    }
                    break;
                }

                case DecorationButtonType::ContextHelp:
                {
                    QColor baseColor;

                    if ( !isInactive )
                        baseColor = QColor(103, 149, 210);
                    else
                        baseColor = inactiveCol;

                    if (!isPressed()) {
                        QLinearGradient grad(QPointF(9, 2), QPointF(9, 16));
                        grad.setColorAt(0, isInactive ? inactiveCol
                        : QColor(103, 149, 210));
                        grad.setColorAt(1, isInactive ? inactiveCol
                        : QColor(93, 135, 190));

                        painter->setBrush(QBrush(grad));
                        painter->setPen(baseColor.darker(140));
                        painter->drawEllipse(QRectF(1, 1, 16, 16));
                        if (backgroundColor.isValid())
                        {
                            painter->setPen(baseColor.darker(140));
                            painter->setBrush(backgroundColor);
                            qreal r = static_cast<qreal>(7)
                            + static_cast<qreal>(2) * m_animation->currentValue().toReal();
                            QPointF c(static_cast<qreal>(9), static_cast<qreal>(9));
                            painter->drawEllipse(c, r, r);
                        }
                    }
                    if (isPressed() || isHovered()) {
                        if ((isPressed()) && backgroundColor.isValid())
                        {
                            painter->setPen(baseColor.darker(140));
                            painter->setBrush(backgroundColor);
                            painter->drawEllipse(QRectF(0, 0, 18, 18));
                        }
                        painter->setPen(pen);
                        painter->setBrush(Qt::NoBrush);

                        QPainterPath path;
                        path.moveTo(5, 6);
                        path.arcTo(QRectF(5, 3.5, 8, 5), 180, -180);
                        path.cubicTo(QPointF(12.5, 9.5), QPointF(9, 7.5), QPointF(9, 11.5));
                        painter->drawPath(path);

                        painter->drawPoint(9, 15);
                    }

                    break;
                }

                default: break;

            }

        }

    }

    //__________________________________________________________________
    void Button::drawIconOxygen( QPainter *painter ) const
    {
        painter->setRenderHints(QPainter::Antialiasing);

        /*
         *   scale painter so that its window matches QRect(-1, -1, 20, 20)
         *   this makes all further rendering and scaling simpler
         *   all further rendering is performed inside QRect(0, 0, 18, 18)
         */
        const QRectF rect = geometry().marginsRemoved(m_padding);
        painter->translate(rect.topLeft());

        const qreal width(rect.width());
        painter->scale(width/20, width/20);
        painter->translate(1, 1);

        // render background
        const QColor backgroundColor(this->backgroundColor());

        auto d = qobject_cast<Decoration*>(decoration());
        bool isInactive(d && !d->window()->isActive()
        && !isHovered() && !isPressed()
        && m_animation->state() != QAbstractAnimation::Running);
        QColor inactiveCol(Qt::gray);
        if (isInactive)
        {
            int gray = qGray(d->titleBarColor().rgb());
            if (gray <= 200) {
                gray += 55;
                gray = qMax(gray, 115);
            }
            else gray -= 45;
            inactiveCol = QColor(gray, gray, gray);
        }

       // QColor symbolColor;
       // symbolColor = Qt::black;
        // render mark
        const QColor foregroundColor(this->foregroundColor(inactiveCol));
        if (foregroundColor.isValid())
        {
            //  QColor base;
            QColor color;
            QColor color1;
            QColor symbolColor;
            QColor symbolColor1;
            const bool sunken = isPressed() || isChecked();

            if (d && qGray(d->titleBarColor().rgb()) > 130)
            {
                color = Qt::white;
                color1 = QColor(239, 240, 241);
                symbolColor = Qt::black;
                symbolColor1 = Qt::white;
            }
            else
            {
                color = QColor(122, 122, 122);
                color1 = QColor(92, 92, 92);
                symbolColor = Qt::white;
                symbolColor1 = Qt::black;
            }

            // setup painter
            QPen pen(symbolColor);
            pen.setWidthF(qMax(1.5 * 21 / width, pen.widthF()));
            QPen pens(symbolColor1);
            pens.setWidthF(qMax(1.5 * 21 / width, pen.widthF()));

            switch (type())
            {

                case DecorationButtonType::Close:
                {
                    QColor glow;
                    glow = QColor(191, 3, 3);
                    QPen penGlow(glow);
                    penGlow.setWidthF(qMax(1.5 * 21 / width, pen.widthF()));

                    painter->setRenderHints(QPainter::Antialiasing);
                    painter->setPen(Qt::NoPen);

                    // button shadow
                    if (!isHovered()) {
                        painter->save();
                        drawShadow(painter, Qt::black, 21);
                        painter->restore();
                    }

                    // button shadow
                    if (isHovered()) {
                        painter->save();
                        drawOuterGlow(painter, glow, 21);
                        painter->restore();
                    }
                    // plain background
                    QLinearGradient lg(0, 3, 0 , (14.5 + 3));
                    if (sunken) {
                        lg.setColorAt(1, color);
                        lg.setColorAt(0, color1.darker(130));
                    } else {
                        lg.setColorAt(0, color);
                        lg.setColorAt(1, color1.darker(130));
                    }

                    const QRectF r(3.3, 3, 14.5, 14.5);
                    painter->setBrush(lg);
                    painter->drawEllipse(r);


                    // outline circle
                    const qreal penWidth(0.2);
                    QLinearGradient lgc(0, 3, 0, (2.0 * 14.5 + 3));
                    lgc.setColorAt(0, color.lighter(110));
                    lgc.setColorAt(1, color.darker(110));
                    const QRectF rc(3.3 + penWidth, (3 + penWidth), (14.5 - penWidth), (14.5 - penWidth));
                    painter->setPen(QPen(lgc, penWidth));
                    painter->setBrush(Qt::NoBrush);
                    painter->drawEllipse(rc);

                    if (foregroundColor.isValid())
                    {
                        // plain background
                        QLinearGradient lg(0, 3, 0 , (14.5 + 3));
                        if (sunken) {
                            lg.setColorAt(1, color);
                            lg.setColorAt(0, color1.darker(130));
                        } else {
                            lg.setColorAt(0, color);
                            lg.setColorAt(1, color1.darker(130));
                        }

                        const QRectF r(3.3, 3, 14.5, 14.5);
                        painter->setBrush(lg);
                        painter->drawEllipse(r);


                        // outline circle
                        const qreal penWidth(0.2);
                        QLinearGradient lgc(0, 3, 0, (2.0 * 14.5 + 3));
                        lgc.setColorAt(0, color.lighter(110));
                        lgc.setColorAt(1, color.darker(110));
                        const QRectF rc(3.3 + penWidth, (3 + penWidth), (14.5 - penWidth), (14.5 - penWidth));
                        painter->setPen(QPen(lgc, penWidth));
                        painter->setBrush(Qt::NoBrush);
                        painter->drawEllipse(rc);
                    }
                    painter->setPen(pens);
                    painter->setBrush(symbolColor1);
                    painter->drawLine(QPointF(8.5, 8.5), QPointF(14.5, 14.5));
                    painter->drawLine(QPointF(14.5, 8.5), QPointF(8.5, 14.5));

                    painter->setPen(pen);
                    painter->setBrush(symbolColor);
                    painter->drawLine(QPointF(7.5, 7.5), QPointF(13.5, 13.5));
                    painter->drawLine(QPointF(13.5, 7.5), QPointF(7.5, 13.5));

                    if (isHovered()) {
                        painter->setPen(penGlow);
                        painter->setBrush(glow);
                        painter->drawLine(QPointF(7.5, 7.5), QPointF(13.5, 13.5));
                        painter->drawLine(QPointF(13.5, 7.5), QPointF(7.5, 13.5));
                    }

                    break;
                }

                case DecorationButtonType::Maximize:
                {
                    QColor glow;
                    glow = QColor(36, 191, 57);
                    QPen penGlow(glow);
                    penGlow.setWidthF(qMax(2.1 * 21 / width, pen.widthF()));

                    painter->setRenderHints(QPainter::Antialiasing);
                    painter->setPen(Qt::NoPen);

                    // button shadow
                    if (!isHovered()) {
                        painter->save();
                        drawShadow(painter, Qt::black, 21);
                        painter->restore();
                    }

                    // button shadow
                    if (isHovered()) {
                        painter->save();
                        drawOuterGlow(painter, glow, 21);
                        painter->restore();
                    }
                    // plain background
                    QLinearGradient lg(0, 3, 0 , (14.5 + 3));
                    if (sunken) {
                        lg.setColorAt(1, color);
                        lg.setColorAt(0, color1.darker(130));
                    } else {
                        lg.setColorAt(0, color);
                        lg.setColorAt(1, color1.darker(130));
                    }

                    const QRectF r(3.3, 3, 14.5, 14.5);
                    painter->setBrush(lg);
                    painter->drawEllipse(r);


                    // outline circle
                    const qreal penWidth(0.2);
                    QLinearGradient lgc(0, 3, 0, (2.0 * 14.5 + 3));
                    lgc.setColorAt(0, color.lighter(110));
                    lgc.setColorAt(1, color.darker(110));
                    const QRectF rc(3.3 + penWidth, (3 + penWidth), (14.5 - penWidth), (14.5 - penWidth));
                    painter->setPen(QPen(lgc, penWidth));
                    painter->setBrush(Qt::NoBrush);
                    painter->drawEllipse(rc);

                    if (foregroundColor.isValid())
                    {
                        // plain background
                        QLinearGradient lg(0, 3, 0 , (14.5 + 3));
                        if (sunken) {
                            lg.setColorAt(1, color);
                            lg.setColorAt(0, color1.darker(130));
                        } else {
                            lg.setColorAt(0, color);
                            lg.setColorAt(1, color1.darker(130));
                        }

                        const QRectF r(3.3, 3, 14.5, 14.5);
                        painter->setBrush(lg);
                        painter->drawEllipse(r);


                        // outline circle
                        const qreal penWidth(0.2);
                        QLinearGradient lgc(0, 3, 0, (2.0 * 14.5 + 3));
                        lgc.setColorAt(0, color.lighter(110));
                        lgc.setColorAt(1, color.darker(110));
                        const QRectF rc(3.3 + penWidth, (3 + penWidth), (14.5 - penWidth), (14.5 - penWidth));
                        painter->setPen(QPen(lgc, penWidth));
                        painter->setBrush(Qt::NoBrush);
                        painter->drawEllipse(rc);
                    }
                    painter->setPen(pens);
                    painter->setBrush(symbolColor1);
                    if (decoration()->window()->isMaximized()) {
                        painter->drawPolygon(QPolygonF() << QPointF(8.5, 11.5) << QPointF(11.5, 8.5) << QPointF(14.5, 11.5) << QPointF(11.5, 14.5));

                    } else {
                        painter->drawPolyline(QPolygonF() << QPointF(8.5, 12.5) << QPointF(11.5, 9.5) << QPointF(14.5, 12.5));
                    }

                    painter->setPen(pen);
                    painter->setBrush(symbolColor);
                    if (decoration()->window()->isMaximized()) {
                        painter->drawPolygon(QPolygonF() << QPointF(7.5, 10.5) << QPointF(10.5, 7.5) << QPointF(13.5, 10.5) << QPointF(10.5, 13.5));

                    } else {
                        painter->drawPolyline(QPolygonF() << QPointF(7.5, 11.5) << QPointF(10.5, 8.5) << QPointF(13.5, 11.5));
                    }

                    if (isHovered()) {
                        painter->setPen(penGlow);
                        painter->setBrush(glow);
                        if (decoration()->window()->isMaximized()) {
                            painter->drawPolygon(QPolygonF() << QPointF(7.5, 10.5) << QPointF(10.5, 7.5) << QPointF(13.5, 10.5) << QPointF(10.5, 13.5));

                        } else {
                            painter->drawPolyline(QPolygonF() << QPointF(7.5, 11.5) << QPointF(10.5, 8.5) << QPointF(13.5, 11.5));
                        }
                    }

                    break;
                }

                case DecorationButtonType::Minimize:
                {
                    QColor glow;
                    glow = QColor(243, 176, 43);
                    QPen penGlow(glow);
                    penGlow.setWidthF(qMax(2.1 * 21 / width, pen.widthF()));

                    painter->setRenderHints(QPainter::Antialiasing);
                    painter->setPen(Qt::NoPen);

                    // button shadow
                    if (color.isValid()) {
                        painter->save();
                        drawShadow(painter, Qt::black, 21);
                        painter->restore();
                    }

                    // button shadow
                    if (isHovered()) {
                        painter->save();
                        drawOuterGlow(painter, glow, 21);
                        painter->restore();
                    }
                    // plain background
                    QLinearGradient lg(0, 3, 0 , (14.5 + 3));
                    if (sunken) {
                        lg.setColorAt(1, color);
                        lg.setColorAt(0, color1.darker(130));
                    } else {
                        lg.setColorAt(0, color);
                        lg.setColorAt(1, color1.darker(130));
                    }

                    const QRectF r(3.3, 3, 14.5, 14.5);
                    painter->setBrush(lg);
                    painter->drawEllipse(r);


                    // outline circle
                    const qreal penWidth(0.2);
                    QLinearGradient lgc(0, 3, 0, (2.0 * 14.5 + 3));
                    lgc.setColorAt(0, color.lighter(110));
                    lgc.setColorAt(1, color.darker(110));
                    const QRectF rc(3.3 + penWidth, (3 + penWidth), (14.5 - penWidth), (14.5 - penWidth));
                    painter->setPen(QPen(lgc, penWidth));
                    painter->setBrush(Qt::NoBrush);
                    painter->drawEllipse(rc);

                    if (foregroundColor.isValid())
                    {
                        // plain background
                        QLinearGradient lg(0, 3, 0 , (14.5 + 3));
                        if (sunken) {
                            lg.setColorAt(1, color);
                            lg.setColorAt(0, color1.darker(130));
                        } else {
                            lg.setColorAt(0, color);
                            lg.setColorAt(1, color1.darker(130));
                        }

                        const QRectF r(3.3, 3, 14.5, 14.5);
                        painter->setBrush(lg);
                        painter->drawEllipse(r);


                        // outline circle
                        const qreal penWidth(0.2);
                        QLinearGradient lgc(0, 3, 0, (2.0 * 14.5 + 3));
                        lgc.setColorAt(0, color.lighter(110));
                        lgc.setColorAt(1, color.darker(110));
                        const QRectF rc(3.3 + penWidth, (3 + penWidth), (14.5 - penWidth), (14.5 - penWidth));
                        painter->setPen(QPen(lgc, penWidth));
                        painter->setBrush(Qt::NoBrush);
                        painter->drawEllipse(rc);
                    }
                    painter->setPen(pens);
                    painter->setBrush(symbolColor1);
                    painter->drawPolyline(QPolygonF() << QPointF(8.5, 10.5) << QPointF(11.5, 13.5) << QPointF(14.5, 10.5));

                    painter->setPen(pen);
                    painter->setBrush(symbolColor);
                    painter->drawPolyline(QPolygonF() << QPointF(7.5, 9.5) << QPointF(10.5, 12.5) << QPointF(13.5, 9.5));

                    if (isHovered()) {
                        painter->setPen(penGlow);
                        painter->setBrush(glow);
                        painter->drawPolyline(QPolygonF() << QPointF(7.5, 9.5) << QPointF(10.5, 12.5) << QPointF(13.5, 9.5));
                    }
                    break;
                }

                case DecorationButtonType::OnAllDesktops:
                {
                    QColor glow;
                    glow = QColor(142, 203, 233);
                    QPen penGlow(glow);
                    penGlow.setWidthF(qMax(2.1 * 21 / width, pen.widthF()));

                    painter->setRenderHints(QPainter::Antialiasing);
                    painter->setPen(Qt::NoPen);

                    // button shadow
                    if (color.isValid()) {
                        painter->save();
                        drawShadow(painter, Qt::black, 21);
                        painter->restore();
                    }

                    // button shadow
                    if (isHovered()) {
                        painter->save();
                        drawOuterGlow(painter, glow, 21);
                        painter->restore();
                    }
                    // plain background
                    QLinearGradient lg(0, 3, 0 , (14.5 + 3));
                    if (sunken) {
                        lg.setColorAt(1, color);
                        lg.setColorAt(0, color1.darker(130));
                    } else {
                        lg.setColorAt(0, color);
                        lg.setColorAt(1, color1.darker(130));
                    }

                    const QRectF r(3.3, 3, 14.5, 14.5);
                    painter->setBrush(lg);
                    painter->drawEllipse(r);


                    // outline circle
                    const qreal penWidth(0.2);
                    QLinearGradient lgc(0, 3, 0, (2.0 * 14.5 + 3));
                    lgc.setColorAt(0, color.lighter(110));
                    lgc.setColorAt(1, color.darker(110));
                    const QRectF rc(3.3 + penWidth, (3 + penWidth), (14.5 - penWidth), (14.5 - penWidth));
                    painter->setPen(QPen(lgc, penWidth));
                    painter->setBrush(Qt::NoBrush);
                    painter->drawEllipse(rc);

                    if (foregroundColor.isValid())
                    {
                        // plain background
                        QLinearGradient lg(0, 3, 0 , (14.5 + 3));
                        if (sunken) {
                            lg.setColorAt(1, color);
                            lg.setColorAt(0, color1.darker(130));
                        } else {
                            lg.setColorAt(0, color);
                            lg.setColorAt(1, color1.darker(130));
                        }

                        const QRectF r(3.3, 3, 14.5, 14.5);
                        painter->setBrush(lg);
                        painter->drawEllipse(r);


                        // outline circle
                        const qreal penWidth(0.2);
                        QLinearGradient lgc(0, 3, 0, (2.0 * 14.5 + 3));
                        lgc.setColorAt(0, color.lighter(110));
                        lgc.setColorAt(1, color.darker(110));
                        const QRectF rc(3.3 + penWidth, (3 + penWidth), (14.5 - penWidth), (14.5 - penWidth));
                        painter->setPen(QPen(lgc, penWidth));
                        painter->setBrush(Qt::NoBrush);
                        painter->drawEllipse(rc);
                    }
                    painter->setPen(pens);
                    painter->setBrush(symbolColor1);
                    painter->drawPoint(QPointF(11.5, 11.5));

                    painter->setPen(pen);
                    painter->setBrush(symbolColor);
                    painter->drawPoint(QPointF(10.5, 10.5));

                    if (isHovered()) {
                        painter->setPen(penGlow);
                        painter->setBrush(glow);
                        painter->drawPoint(QPointF(10.5, 10.5));
                    }
                    break;
                }

                case DecorationButtonType::KeepBelow:
                {
                    QColor glow;
                    glow = QColor(142, 203, 233);
                    QPen penGlow(glow);
                    penGlow.setWidthF(qMax(2.1 * 21 / width, pen.widthF()));

                    painter->setRenderHints(QPainter::Antialiasing);
                    painter->setPen(Qt::NoPen);

                    // button shadow
                    if (color.isValid()) {
                        painter->save();
                        drawShadow(painter, Qt::black, 21);
                        painter->restore();
                    }

                    // button shadow
                    if (isHovered()) {
                        painter->save();
                        drawOuterGlow(painter, glow, 21);
                        painter->restore();
                    }
                    // plain background
                    QLinearGradient lg(0, 3, 0 , (14.5 + 3));
                    if (sunken) {
                        lg.setColorAt(1, color);
                        lg.setColorAt(0, color1.darker(130));
                    } else {
                        lg.setColorAt(0, color);
                        lg.setColorAt(1, color1.darker(130));
                    }

                    const QRectF r(3.3, 3, 14.5, 14.5);
                    painter->setBrush(lg);
                    painter->drawEllipse(r);


                    // outline circle
                    const qreal penWidth(0.2);
                    QLinearGradient lgc(0, 3, 0, (2.0 * 14.5 + 2.7));
                    lgc.setColorAt(0, color.lighter(110));
                    lgc.setColorAt(1, color.darker(110));
                    const QRectF rc(3.3 + penWidth, (3 + penWidth), (14.5 - penWidth), (14.5 - penWidth));
                    painter->setPen(QPen(lgc, penWidth));
                    painter->setBrush(Qt::NoBrush);
                    painter->drawEllipse(rc);

                    if (foregroundColor.isValid())
                    {
                        // plain background
                        QLinearGradient lg(0, 3, 0 , (14.5 + 2.7));
                        if (sunken) {
                            lg.setColorAt(1, color);
                            lg.setColorAt(0, color1.darker(130));
                        } else {
                            lg.setColorAt(0, color);
                            lg.setColorAt(1, color1.darker(130));
                        }

                        const QRectF r(3.3, 3, 14.5, 14.5);
                        painter->setBrush(lg);
                        painter->drawEllipse(r);


                        // outline circle
                        const qreal penWidth(0.2);
                        QLinearGradient lgc(0, 3, 0, (2.0 * 14.5 + 3));
                        lgc.setColorAt(0, color.lighter(110));
                        lgc.setColorAt(1, color.darker(110));
                        const QRectF rc(3.3 + penWidth, (3 + penWidth), (14.5 - penWidth), (14.5 - penWidth));
                        painter->setPen(QPen(lgc, penWidth));
                        painter->setBrush(Qt::NoBrush);
                        painter->drawEllipse(rc);
                    }
                    painter->setPen(pens);
                    painter->setBrush(symbolColor1);
                    painter->drawPolyline(QPolygonF() << QPointF(8.5, 12) << QPointF(11.5, 15) << QPointF(14.5, 12));
                    painter->drawPolyline(QPolygonF() << QPointF(8.5, 8) << QPointF(11.5, 11) << QPointF(14.5, 8));

                    painter->setPen(pen);
                    painter->setBrush(symbolColor);
                    painter->drawPolyline(QPolygonF() << QPointF(7.5, 11) << QPointF(10.5, 14) << QPointF(13.5, 11));
                    painter->drawPolyline(QPolygonF() << QPointF(7.5, 7) << QPointF(10.5, 10) << QPointF(13.5, 7));

                    if (isHovered()) {
                        painter->setPen(penGlow);
                        painter->setBrush(glow);
                        painter->drawPolyline(QPolygonF() << QPointF(7.5, 11) << QPointF(10.5, 14) << QPointF(13.5, 11));
                        painter->drawPolyline(QPolygonF() << QPointF(7.5, 7) << QPointF(10.5, 10) << QPointF(13.5, 7));
                    }
                    break;

                }

                case DecorationButtonType::KeepAbove:
                {
                    QColor glow;
                    glow = QColor(142, 203, 233);
                    QPen penGlow(glow);
                    penGlow.setWidthF(qMax(2.1 * 21 / width, pen.widthF()));

                    painter->setRenderHints(QPainter::Antialiasing);
                    painter->setPen(Qt::NoPen);

                    // button shadow
                    if (color.isValid()) {
                        painter->save();
                        drawShadow(painter, Qt::black, 21);
                        painter->restore();
                    }

                    // button shadow
                    if (isHovered()) {
                        painter->save();
                        drawOuterGlow(painter, glow, 21);
                        painter->restore();
                    }
                    // plain background
                    QLinearGradient lg(0, 3, 0 , (14.5 + 3));
                    if (sunken) {
                        lg.setColorAt(1, color);
                        lg.setColorAt(0, color1.darker(130));
                    } else {
                        lg.setColorAt(0, color);
                        lg.setColorAt(1, color1.darker(130));
                    }

                    const QRectF r(3.3, 3, 14.5, 14.5);
                    painter->setBrush(lg);
                    painter->drawEllipse(r);


                    // outline circle
                    const qreal penWidth(0.2);
                    QLinearGradient lgc(0, 3, 0, (2.0 * 14.5 + 2.7));
                    lgc.setColorAt(0, color.lighter(110));
                    lgc.setColorAt(1, color.darker(110));
                    const QRectF rc(3.3 + penWidth, (3 + penWidth), (14.5 - penWidth), (14.5 - penWidth));
                    painter->setPen(QPen(lgc, penWidth));
                    painter->setBrush(Qt::NoBrush);
                    painter->drawEllipse(rc);

                    if (foregroundColor.isValid())
                    {
                        // plain background
                        QLinearGradient lg(0, 3, 0 , (14.5 + 2.7));
                        if (sunken) {
                            lg.setColorAt(1, color);
                            lg.setColorAt(0, color1.darker(130));
                        } else {
                            lg.setColorAt(0, color);
                            lg.setColorAt(1, color1.darker(130));
                        }

                        const QRectF r(3.3, 3, 14.5, 14.5);
                        painter->setBrush(lg);
                        painter->drawEllipse(r);


                        // outline circle
                        const qreal penWidth(0.2);
                        QLinearGradient lgc(0, 3, 0, (2.0 * 14.5 + 3));
                        lgc.setColorAt(0, color.lighter(110));
                        lgc.setColorAt(1, color.darker(110));
                        const QRectF rc(3.3 + penWidth, (3 + penWidth), (14.5 - penWidth), (14.5 - penWidth));
                        painter->setPen(QPen(lgc, penWidth));
                        painter->setBrush(Qt::NoBrush);
                        painter->drawEllipse(rc);
                    }
                    painter->setPen(pens);
                    painter->setBrush(symbolColor1);
                    painter->drawPolyline(QPolygonF() << QPointF(8.5, 15) << QPointF(11.5, 11) << QPointF(14.5, 15));
                    painter->drawPolyline(QPolygonF() << QPointF(8.5, 11) << QPointF(11.5, 7) << QPointF(14.5, 11));

                    painter->setPen(pen);
                    painter->setBrush(symbolColor);
                    painter->drawPolyline(QPolygonF() << QPointF(7.5, 14) << QPointF(10.5, 11) << QPointF(13.5, 14));
                    painter->drawPolyline(QPolygonF() << QPointF(7.5, 10) << QPointF(10.5, 7) << QPointF(13.5, 10));

                    if (isHovered()) {
                        painter->setPen(penGlow);
                        painter->setBrush(glow);
                        painter->drawPolyline(QPolygonF() << QPointF(7.5, 14) << QPointF(10.5, 11) << QPointF(13.5, 14));
                        painter->drawPolyline(QPolygonF() << QPointF(7.5, 10) << QPointF(10.5, 7) << QPointF(13.5, 10));
                    }
                    break;
                }


                case DecorationButtonType::ApplicationMenu:
                {
                    QColor glow;
                    glow = QColor(142, 203, 233);
                    QPen penGlow(glow);
                    penGlow.setWidthF(qMax(2.1 * 21 / width, pen.widthF()));

                    painter->setRenderHints(QPainter::Antialiasing);
                    painter->setPen(Qt::NoPen);

                    // button shadow
                    if (color.isValid()) {
                        painter->save();
                        drawShadow(painter, Qt::black, 21);
                        painter->restore();
                    }

                    // button shadow
                    if (isHovered()) {
                        painter->save();
                        drawOuterGlow(painter, glow, 21);
                        painter->restore();
                    }
                    // plain background
                    QLinearGradient lg(0, 3, 0 , (14.5 + 3));
                    if (sunken) {
                        lg.setColorAt(1, color);
                        lg.setColorAt(0, color1.darker(130));
                    } else {
                        lg.setColorAt(0, color);
                        lg.setColorAt(1, color1.darker(130));
                    }

                    const QRectF r(3.3, 3, 14.5, 14.5);
                    painter->setBrush(lg);
                    painter->drawEllipse(r);


                    // outline circle
                    const qreal penWidth(0.2);
                    QLinearGradient lgc(0, 3, 0, (2.0 * 14.5 + 2.7));
                    lgc.setColorAt(0, color.lighter(110));
                    lgc.setColorAt(1, color.darker(110));
                    const QRectF rc(3.3 + penWidth, (3 + penWidth), (14.5 - penWidth), (14.5 - penWidth));
                    painter->setPen(QPen(lgc, penWidth));
                    painter->setBrush(Qt::NoBrush);
                    painter->drawEllipse(rc);

                    if (foregroundColor.isValid())
                    {
                        // plain background
                        QLinearGradient lg(0, 3, 0 , (14.5 + 2.7));
                        if (sunken) {
                            lg.setColorAt(1, color);
                            lg.setColorAt(0, color1.darker(130));
                        } else {
                            lg.setColorAt(0, color);
                            lg.setColorAt(1, color1.darker(130));
                        }

                        const QRectF r(3.3, 3, 14.5, 14.5);
                        painter->setBrush(lg);
                        painter->drawEllipse(r);


                        // outline circle
                        const qreal penWidth(0.2);
                        QLinearGradient lgc(0, 3, 0, (2.0 * 14.5 + 3));
                        lgc.setColorAt(0, color.lighter(110));
                        lgc.setColorAt(1, color.darker(110));
                        const QRectF rc(3.3 + penWidth, (3 + penWidth), (14.5 - penWidth), (14.5 - penWidth));
                        painter->setPen(QPen(lgc, penWidth));
                        painter->setBrush(Qt::NoBrush);
                        painter->drawEllipse(rc);
                    }
                    painter->setPen(pens);
                    painter->setBrush(symbolColor1);
                    painter->drawLine(QPointF(8.5, 8), QPointF(14.5, 8));
                    painter->drawLine(QPointF(8.5, 11), QPointF(14.5, 11));
                    painter->drawLine(QPointF(8.5, 14), QPointF(14.5, 14));

                    painter->setPen(pen);
                    painter->setBrush(symbolColor);
                    painter->drawLine(QPointF(7.5, 8), QPointF(13.5, 8));
                    painter->drawLine(QPointF(7.5, 11), QPointF(13.5, 11));
                    painter->drawLine(QPointF(7.5, 14), QPointF(13.5, 14));
                    if (isHovered()) {
                        painter->setPen(penGlow);
                        painter->setBrush(glow);
                        painter->drawLine(QPointF(7.5, 8), QPointF(13.5, 8));
                        painter->drawLine(QPointF(7.5, 11), QPointF(13.5, 11));
                        painter->drawLine(QPointF(7.5, 14), QPointF(13.5, 14));
                    }
                    break;
                }

                case DecorationButtonType::ContextHelp:
                {
                    QColor glow;
                    glow = QColor(142, 203, 233);
                    QPen penGlow(glow);
                    penGlow.setWidthF(qMax(2.1 * 21 / width, pen.widthF()));

                    painter->setRenderHints(QPainter::Antialiasing);
                    painter->setPen(Qt::NoPen);

                    // button shadow
                    if (color.isValid()) {
                        painter->save();
                        drawShadow(painter, Qt::black, 21);
                        painter->restore();
                    }

                    // button shadow
                    if (isHovered()) {
                        painter->save();
                        drawOuterGlow(painter, glow, 21);
                        painter->restore();
                    }
                    // plain background
                    QLinearGradient lg(0, 3, 0 , (14.5 + 3));
                    if (sunken) {
                        lg.setColorAt(1, color);
                        lg.setColorAt(0, color1.darker(130));
                    } else {
                        lg.setColorAt(0, color);
                        lg.setColorAt(1, color1.darker(130));
                    }

                    const QRectF r(3.3, 3, 14.5, 14.5);
                    painter->setBrush(lg);
                    painter->drawEllipse(r);


                    // outline circle
                    const qreal penWidth(0.2);
                    QLinearGradient lgc(0, 3, 0, (2.0 * 14.5 + 2.7));
                    lgc.setColorAt(0, color.lighter(110));
                    lgc.setColorAt(1, color.darker(110));
                    const QRectF rc(3.3 + penWidth, (3 + penWidth), (14.5 - penWidth), (14.5 - penWidth));
                    painter->setPen(QPen(lgc, penWidth));
                    painter->setBrush(Qt::NoBrush);
                    painter->drawEllipse(rc);

                    if (foregroundColor.isValid())
                    {
                        // plain background
                        QLinearGradient lg(0, 3, 0 , (14.5 + 2.7));
                        if (sunken) {
                            lg.setColorAt(1, color);
                            lg.setColorAt(0, color1.darker(130));
                        } else {
                            lg.setColorAt(0, color);
                            lg.setColorAt(1, color1.darker(130));
                        }

                        const QRectF r(3.3, 3, 14.5, 14.5);
                        painter->setBrush(lg);
                        painter->drawEllipse(r);


                        // outline circle
                        const qreal penWidth(0.2);
                        QLinearGradient lgc(0, 3, 0, (2.0 * 14.5 + 3));
                        lgc.setColorAt(0, color.lighter(110));
                        lgc.setColorAt(1, color.darker(110));
                        const QRectF rc(3.3 + penWidth, (3 + penWidth), (14.5 - penWidth), (14.5 - penWidth));
                        painter->setPen(QPen(lgc, penWidth));
                        painter->setBrush(Qt::NoBrush);
                        painter->drawEllipse(rc);
                    }
                    painter->setPen(pens);
                    painter->setBrush(symbolColor1);
                    QPainterPath path1;
                    path1.moveTo(5, 6);
                    path1.arcTo(QRectF(5, 3.5, 8, 5), 180, -180);
                    path1.cubicTo(QPointF(12.5, 9.5), QPointF(9, 7.5), QPointF(9, 11.5));
                    painter->drawPath(path1);

                    painter->drawPoint(9, 15);

                    painter->setPen(pen);
                    painter->setBrush(symbolColor);
                    QPainterPath path2;
                    path2.moveTo(5, 6);
                    path2.arcTo(QRectF(5, 3.5, 8, 5), 180, -180);
                    path2.cubicTo(QPointF(12.5, 9.5), QPointF(9, 7.5), QPointF(9, 11.5));
                    painter->drawPath(path2);

                    painter->drawPoint(9, 15);
                    if (isHovered()) {
                        painter->setPen(penGlow);
                        painter->setBrush(glow);
                        QPainterPath path3;
                        path3.moveTo(5, 6);
                        path3.arcTo(QRectF(5, 3.5, 8, 5), 180, -180);
                        path3.cubicTo(QPointF(12.5, 9.5), QPointF(9, 7.5), QPointF(9, 11.5));
                        painter->drawPath(path3);

                        painter->drawPoint(9, 15);
                    }
                    break;
                }

                default: break;

            }

        }

    }

    //__________________________________________________________________
    QColor Button::foregroundColor(const QColor& inactiveCol) const
    {
        auto d = qobject_cast<Decoration*>(decoration());
        if (!d || d->internalSettings()->buttonStyle() != 3) {
            QColor col;
            if (d && !d->window()->isActive()
                && !isHovered() && !isPressed()
                && m_animation->state() != QAbstractAnimation::Running)
            {
                int v = qGray(inactiveCol.rgb());
                if (v > 127) v -= 127;
                else v += 128;
                col = QColor(v, v, v);
            }
            else
            {
                if (d && qGray(d->titleBarColor().rgb()) > 100)
                    col = QColor(250, 250, 250);
                else
                    col = QColor(40, 40, 40);
            }
            return col;
        }
        else if (!d) {

            return QColor();

        } else if (isPressed()) {

            return d->titleBarColor();

        /*} else if (type() == DecorationButtonType::Close && d->internalSettings()->outlineCloseButton()) {

            return d->titleBarColor();*/

        } else if ((type() == DecorationButtonType::KeepBelow || type() == DecorationButtonType::KeepAbove) && isChecked()) {

            return d->titleBarColor();

        } else if (m_animation->state() == QAbstractAnimation::Running) {

            return KColorUtils::mix(d->fontColor(), d->titleBarColor(), m_opacity);

        } else if (isHovered()) {

            return d->titleBarColor();

        } else {

            return d->fontColor();

        }

    }

    //__________________________________________________________________
    QColor Button::backgroundColor() const
    {
        auto d = qobject_cast<Decoration*>(decoration());
        if (!d) {

            return QColor();

        }

        if (d->internalSettings()->buttonStyle() != 3) {
            if (isPressed()) {

                QColor col;
                if (type() == DecorationButtonType::Close)
                {
                    if (qGray(d->titleBarColor().rgb()) > 100)
                        col = QColor(254, 73, 66);
                    else
                        col = QColor(240, 77, 80);
                }
                else if (type() == DecorationButtonType::Maximize)
                {
                    if (qGray(d->titleBarColor().rgb()) > 100)
                        col = isChecked() ? QColor(0, 188, 154) : QColor(7, 201, 33);
                    else
                        col = isChecked() ? QColor(0, 188, 154) : QColor(101, 188, 34);
                }
                else if (type() == DecorationButtonType::Minimize)
                {
                    if (qGray(d->titleBarColor().rgb()) > 100)
                        col = QColor(233, 160, 13);
                    else
                        col = QColor(227, 185, 59);
                }
                else if (type() == DecorationButtonType::ApplicationMenu) {
                    if (qGray(d->titleBarColor().rgb()) > 100)
                        col = QColor(220, 124, 64);
                    else
                        col = QColor(240, 139, 96);
                }
                else {
                    if (qGray(d->titleBarColor().rgb()) > 100)
                        col = QColor(83, 121, 170);
                    else
                        col = QColor(110, 136, 180);
                }
                if (col.isValid())
                    return col;
                else return KColorUtils::mix(d->titleBarColor(), d->fontColor(), 0.3);

            } else if (m_animation->state() == QAbstractAnimation::Running) {

                QColor col;
                if (type() == DecorationButtonType::Close)
                {
                    if (qGray(d->titleBarColor().rgb()) > 100)
                        col = QColor(254, 95, 87);
                    else
                        col = QColor(240, 96, 97);
                }
                else if (type() == DecorationButtonType::Maximize)
                {
                    if (qGray(d->titleBarColor().rgb()) > 100)
                        col = isChecked() ? QColor(64, 188, 168) : QColor(39, 201, 63);
                    else
                        col = isChecked() ? QColor(64, 188, 168) : QColor(116, 188, 64);
                }
                else if (type() == DecorationButtonType::Minimize)
                {
                    if (qGray(d->titleBarColor().rgb()) > 100)
                        col = QColor(233, 172, 41);
                    else
                        col = QColor(227, 191, 78);
                }
                else if (type() == DecorationButtonType::ApplicationMenu) {
                    if (qGray(d->titleBarColor().rgb()) > 100)
                        col = QColor(220, 124, 64);
                    else
                        col = QColor(240, 139, 96);
                }
                else {
                    if (qGray(d->titleBarColor().rgb()) > 100)
                        col = QColor(98, 141, 200);
                    else
                        col = QColor(128, 157, 210);
                }
                if (col.isValid())
                    return col;
                else {

                    col = d->fontColor();
                    col.setAlpha(col.alpha()*m_opacity);
                    return col;

                }

            } else if (isHovered()) {

                QColor col;
                if (type() == DecorationButtonType::Close)
                {
                    if (qGray(d->titleBarColor().rgb()) > 100)
                        col = QColor(254, 95, 87);
                    else
                        col = QColor(240, 96, 97);
                }
                else if (type() == DecorationButtonType::Maximize)
                {
                    if (qGray(d->titleBarColor().rgb()) > 100)
                        col = isChecked() ? QColor(64, 188, 168) : QColor(39, 201, 63);
                    else
                        col = isChecked() ? QColor(64, 188, 168) : QColor(116, 188, 64);
                }
                else if (type() == DecorationButtonType::Minimize)
                {
                    if (qGray(d->titleBarColor().rgb()) > 100)
                        col = QColor(233, 172, 41);
                    else
                        col = QColor(227, 191, 78);
                }
                else if (type() == DecorationButtonType::ApplicationMenu) {
                    if (qGray(d->titleBarColor().rgb()) > 100)
                        col = QColor(220, 124, 64);
                    else
                        col = QColor(240, 139, 96);
                }
                else {
                    if (qGray(d->titleBarColor().rgb()) > 100)
                        col = QColor(98, 141, 200);
                    else
                        col = QColor(128, 157, 210);
                }
                if (col.isValid())
                    return col;
                else return d->fontColor();

            } else {

                return QColor();

            }
        }
        else {
            auto c = d->window();
            if (isPressed()) {

                if (type() == DecorationButtonType::Close) return c->color(ColorGroup::Warning, ColorRole::Foreground);
                else
                {
                    QColor col;
                    if (qGray(d->titleBarColor().rgb()) > 100)
                        col = QColor(0, 0, 0, 190);
                    else
                        col = QColor(255, 255, 255, 210);
                    return col;
                }

            } else if ((type() == DecorationButtonType::KeepBelow || type() == DecorationButtonType::KeepAbove) && isChecked()) {

                    QColor col;
                    if (qGray(d->titleBarColor().rgb()) > 100)
                        col = QColor(0, 0, 0, 165);
                    else
                        col = QColor(255, 255, 255, 180);
                    return col;

            } else if (m_animation->state() == QAbstractAnimation::Running) {

                if (type() == DecorationButtonType::Close)
                {

                    QColor color(c->color(ColorGroup::Warning, ColorRole::Foreground).lighter());
                    color.setAlpha(color.alpha()*m_opacity);
                    return color;

                } else {

                    QColor col;
                    if (qGray(d->titleBarColor().rgb()) > 100)
                        col = QColor(0, 0, 0, 165);
                    else
                        col = QColor(255, 255, 255, 180);
                    col.setAlpha(col.alpha()*m_opacity);
                    return col;

                }

            } else if (isHovered()) {

                if (type() == DecorationButtonType::Close) return c->color(ColorGroup::Warning, ColorRole::Foreground).lighter();
                else
                {

                    QColor col;
                    if (qGray(d->titleBarColor().rgb()) > 100)
                        col = QColor(0, 0, 0, 165);
                    else
                        col = QColor(255, 255, 255, 180);
                    return col;

                }

            } else {

                return QColor();

            }
        }

    }

    //_______________________________________________________________________
    void Button::drawOuterGlow(QPainter *painter, const QColor &color, int size) const
    {
        const QRectF r(0, 0, size, size);
        const qreal m(qreal(size) * 0.5);
        const qreal width(3);
        const qreal glowBias = 0.6;


        const qreal bias(glowBias * qreal(18) / size);

        // k0 is located at width - bias from the outer edge
        const qreal gm(m + bias - 0.9);
        const qreal k0((m - width + bias) / gm);
        QRadialGradient glowGradient(m, m, gm);
        for (int i = 0; i < 8; i++) {
            // k1 grows linearly from k0 to 1.0
            const qreal k1(k0 + qreal(i) * (1.0 - k0) / 8.0);

            // a folows sqrt curve
            const qreal a(1.0 - sqrt(qreal(i) / 8));
            glowGradient.setColorAt(k1, alphaColor(color, a));
        }

        // glow
        painter->save();
        painter->setBrush(glowGradient);
        painter->drawEllipse(r);

        // inside mask
        painter->setCompositionMode(QPainter::CompositionMode_DestinationOut);
        painter->setBrush(Qt::black);
        painter->setPen(Qt::NoPen);
        painter->drawEllipse(r.adjusted(width + 0.5, width + 0.5, -width - 1, -width - 1));
        painter->restore();
    }

    //___________________________________________________________________________________________
    void Button::drawShadow(QPainter *painter, const QColor &color, int size) const
    {
        const qreal m(qreal(size - 2) * 0.5);
        const qreal offset(0.6);
        const qreal k0((m - 4.0) / m);
        const qreal shadowGain = 1.5;

        QRadialGradient shadowGradient(m + 1.0, m + offset + 1.0, m);
        for (int i = 0; i < 8; i++) {
            // sinusoidal gradient
            const qreal k1((k0 * qreal(8 - i) + qreal(i)) * 0.125);
            const qreal a((cos(M_PI * i * 0.125) + 1.0) * 0.30);
            shadowGradient.setColorAt(k1, alphaColor(color, a * shadowGain));
        }

        shadowGradient.setColorAt(1.0, alphaColor(color, 0.0));
        painter->save();
        painter->setBrush(shadowGradient);
        painter->setPen(Qt::NoPen);
        painter->drawEllipse(QRectF(0, 0, size, size));
        painter->restore();
    }


    QColor Button::alphaColor(QColor color, qreal alpha) const
    {
        if (alpha >= 0 && alpha < 1.0) {
            color.setAlphaF(alpha * color.alphaF());
        }
        return color;
    }

    //________________________________________________________________
    void Button::reconfigure()
    {

        // animation
        if (auto d = qobject_cast<Decoration*>(decoration()))
        {
            m_animation->setDuration(d->internalSettings()->animationsDuration());
            setPreferredSize(QSizeF(d->buttonSize(), d->buttonSize()));
        }

    }

    //__________________________________________________________________
    void Button::updateAnimationState(bool hovered)
    {

        auto d = qobject_cast<Decoration*>(decoration());
        if (!(d && d->internalSettings()->animationsEnabled())) return;

        QAbstractAnimation::Direction dir = hovered ? QAbstractAnimation::Forward : QAbstractAnimation::Backward;
        if (m_animation->state() == QAbstractAnimation::Running && m_animation->direction() != dir)
            m_animation->stop();
        m_animation->setDirection(dir);
        if (m_animation->state() != QAbstractAnimation::Running) m_animation->start();

    }

} // namespace
