/***************************************************************************
 *   Copyright (c) 2013 Jan Rheinländer                                    *
 *                                   <jrheinlaender@users.sourceforge.net> *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#ifndef FEM_CONSTRAINT_H
#define FEM_CONSTRAINT_H

#include <App/DocumentObject.h>
#include <App/FeaturePython.h>
#include <App/PropertyLinks.h>
#include <App/PropertyUnits.h>
#include <Base/Vector3D.h>
#include <Mod/Fem/FemGlobal.h>
#include <App/SuppressibleExtension.h>


namespace Fem
{

/**
 * @brief Base class of all Constraint Objects of the Fem module.
 *
 * @details
 *  @ref Constraint isn't intended to be used directly. Actual Constraints
 *  used to specify a simulation are children of this class. The base class
 *  essentially does two things: Most importantely it has a property @ref
 *  Constraint::References which is a list of all sub objects the constraint
 *  applies to. Defining it in the base class exposes a common interface to code
 *  using different constraints.
 *
 *  The second purpose of @ref Constraint is to support the redering to the
 *  screen done by the View Provider @ref FemGui::ViewProviderFemConstraint.
 *  The rendering is decoupled from the objects listed in the @ref References
 *  property by using a point cloud a normal vector and a scale factor which is
 *  generated by this class. The View Provider doesn't know of the references
 *  it just asks @ref Constraint for those values and renders a widget for each
 *  point scaled by the scale factor pointing in the direction of the normal
 *  vector. These values are exposed by the two properties @ref NormalDirection
 *  and @ref Scale and the protected method @ref getPoints(points&, normals&,
 *  scale&).
 */
class FemExport Constraint: public App::DocumentObject, public App::SuppressibleExtension
{
    PROPERTY_HEADER_WITH_OVERRIDE(Fem::Constraint);

public:
    Constraint();
    ~Constraint() override;

    /**
     * @brief List of objects the constraints applies to.
     *
     * @details
     *  This is a list of subobjects (e.g. Faces, Edges, ...) the constraint
     *  applies to. It's only supposed to contain objects of or derived from
     *  Part::Feature. Altering this property triggers a update of @ref
     *  NormalDirection and @ref Scale.
     *
     * @note
     *  Undefined behaviour if a unsupported (not derived from Part::Feature)
     *  Document Object is added to the @References.
     */
    App::PropertyLinkSubList References;

    /**
     * @brief Vector pointing into the effective direction of the constraint.
     *
     * @details
     *  If @ref References contains only one face of a shape than @ref
     *  NormalDirection is the normal vector of that face. If more than one
     *  face is referenced that it is the normal vector of the first face. If
     *  @ref References is empty or doesn't contain a face the value of @ref
     *  NormalDirection is the Z-axis or its previous value.
     */
    App::PropertyVector NormalDirection;

    /**
     * @brief Supposed to reflect the size of the @ref References.
     *
     * @details
     *  This property should be a scale factor for the widgets rendered by the
     *  View Provider but it's always 1. It isn't updated when @ref References
     *  changes.
     */
    App::PropertyFloatConstraint Scale;

    // Read-only (calculated values). These trigger changes in the ViewProvider
    App::PropertyVectorList Points;
    App::PropertyVectorList Normals;

    /**
     * @brief Updates @ref NormalDirection.
     *
     * @details
     *  Updates @ref NormalDirection using new @ref References. It does so by
     *  calling @ref onChanged once with the @ref References property and once
     *  with the @ref Scale property. The second call doesn't do anything.
     *
     * @note
     *  Calling @ref onChanged does touch the Document Object but that flag is
     *  cleared right after the @ref execute call by the recompute mechanism.
     *  See Document::recompute() and DocumentObject::purgeTouched().
     */
    App::DocumentObjectExecReturn* execute() override;

    /**
     * @brief Calculates scale factor based on characteristic length of shape.
     *
     * @details
     *  Used to calculate the scale factor returned by @ref getPoints when the
     */
    double calcSizeFactor(double characLen) const;

    const char* getViewProviderName() const override
    {
        return "FemGui::ViewProviderFemConstraint";
    }

    /**
     * @brief Returns Scale * sizeFactor.
     */
    float getScaleFactor() const;

protected:
    /**
     * @brief Updates NormalDirection if References change.
     */
    void onChanged(const App::Property* prop) override;

    /**
     * @brief Triggers @ref onChanged to update View Provider.
     *
     * @note
     *  This should not be necessary and is properly a bug in the View Provider
     *  of FemConstraint.
     */
    void onDocumentRestored() override;
    void onSettingDocument() override;
    void unsetupObject() override;
    void handleChangedPropertyType(Base::XMLReader& reader,
                                   const char* TypeName,
                                   App::Property* prop) override;

    /**
     * @brief Returns data based on References relevant for rendering widgets.
     *
     * @details
     *  Extracts data from all objects inside References relevant for widget
     *  rendering by the View Provider. This includes the points at which
     *  widgets shall be drawn, a vector per point indicating the direction the
     *  widget should face and a global scale factor for all widgets. Two
     *  vectors of equal length are used to return the points and their normal
     *  vectors. The normal vector of points[i] can be found with the same
     *  index in normals[i].
     *
     * @param[out] points
     *  For each vertex a point equal to the location of that vertex is pushed
     *  into the points vector. For each edge at least to points, the beginning
     *  and the end of the edge, are pushed into the vector. Depending on the
     *  length of the edge more points may be added in between. For each face a
     *  number of points depending on the size of the face and the step size
     *  calculated internally are pushed into the vector.
     * @param[out] normals
     *  For vertexes and edges normal vectors equal to the NormalDirection are
     *  pushed onto the vector. For each point of a face a Base::Vector3d equal
     *  to the normal vector of the face at that position is added to the
     *  vector.
     * @param[out] scale
     *  The scale contains a scale value for the object in References that was
     *  processed last. For calculation various versions of @ref
     *  calcDrawScaleFactor are used.
     *
     * @return
     *  If the calculation of points, normals and scale was successful it
     *  returns true. If an error occurred and the data couldn't be extracted
     *  properly false is returned.
     */
    bool getPoints(std::vector<Base::Vector3d>& points,
                   std::vector<Base::Vector3d>& normals,
                   double* scale) const;

    /**
     * @brief Calculate point of cylindrical face where to render widget.
     *
     * @note
     *  This method is very specific and doesn't require access to member
     *  variables. It should be rewritten at a different place.
     */
    Base::Vector3d getBasePoint(const Base::Vector3d& base,
                                const Base::Vector3d& axis,
                                const App::PropertyLinkSub& location,
                                const double& dist);
    /**
     * @brief Get normal vector of point calculated by @ref getBasePoint.
     *
     * @note
     *  This method is very specific and doesn't require access to member
     *  variables. It should be rewritten at a different place.
     */
    const Base::Vector3d getDirection(const App::PropertyLinkSub& direction);

private:
    /**
     * @brief Symbol size factor determined from the size of the shape.
     */
    double sizeFactor;

    void slotChangedObject(const App::DocumentObject& Obj, const App::Property& Prop);
    boost::signals2::connection connDocChangedObject;
};

using ConstraintPython = App::FeaturePythonT<Constraint>;


}  // namespace Fem


#endif  // FEM_CONSTRAINT_H