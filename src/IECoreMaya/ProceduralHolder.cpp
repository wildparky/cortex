//////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2007-2008, Image Engine Design Inc. All rights reserved.
//
//  Redistribution and use in source and binary forms, with or without
//  modification, are permitted provided that the following conditions are
//  met:
//
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//
//     * Neither the name of Image Engine Design nor the names of any
//       other contributors to this software may be used to endorse or
//       promote products derived from this software without specific prior
//       written permission.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
//  IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
//  PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
//  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
//  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
//  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
//  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
//  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
//  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
//  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//////////////////////////////////////////////////////////////////////////

#include <boost/python.hpp>


#include "IECoreGL/Renderer.h"
#include "IECoreGL/Scene.h"
#include "IECoreGL/BoxPrimitive.h"
#include "IECoreGL/TypedStateComponent.h"
#include "IECoreGL/NameStateComponent.h"
#include "IECoreGL/State.h"
#include "IECoreGL/Camera.h"
#include "IECoreGL/Renderable.h"
#include "IECoreGL/Group.h"
#include "IECoreGL/Primitive.h"

#include "IECoreMaya/ProceduralHolder.h"
#include "IECoreMaya/Convert.h"
#include "IECoreMaya/MayaTypeIds.h"

#include "IECore/VectorOps.h"
#include "IECore/MessageHandler.h"
#include "IECore/SimpleTypedData.h"
#include "IECore/ClassData.h"

#include "maya/MFnNumericAttribute.h"
#include "maya/MFnTypedAttribute.h"
#include "maya/MFnSingleIndexedComponent.h"
#include "maya/MSelectionList.h"
#include "maya/MAttributeSpec.h"
#include "maya/MAttributeIndex.h"
#include "maya/MAttributeSpecArray.h"
#include "maya/MDagPath.h"
#include "maya/MFnStringData.h"

using namespace Imath;
using namespace IECore;
using namespace IECoreMaya;
using namespace boost;

MTypeId ProceduralHolder::id = ProceduralHolderId;
MObject ProceduralHolder::aGLPreview;
MObject ProceduralHolder::aTransparent;
MObject ProceduralHolder::aDrawBound;
MObject ProceduralHolder::aProceduralComponents;

struct ProceduralHolder::MemberData
{
	ComponentsMap m_componentsMap;		
	ComponentToGroupMap m_componentToGroupMap;
};

static IECore::ClassData< ProceduralHolder, ProceduralHolder::MemberData > g_classData;

ProceduralHolder::ComponentsMap &ProceduralHolder::componentsMap()
{
	return g_classData[this].m_componentsMap;
	
}
		
ProceduralHolder::ComponentToGroupMap &ProceduralHolder::componentToGroupMap()
{
	return g_classData[this].m_componentToGroupMap;
}

ProceduralHolder::ProceduralHolder()
	:	m_boundDirty( true ), m_sceneDirty( true )
{
	g_classData.create( this );
}

ProceduralHolder::~ProceduralHolder()
{
}

void ProceduralHolder::postConstructor()
{
	ParameterisedHolderComponentShape::postConstructor();
	setRenderable( true );
}

void *ProceduralHolder::creator()
{
	return new ProceduralHolder;
}
			 
MStatus ProceduralHolder::initialize()
{	
	MStatus s = inheritAttributesFrom( ParameterisedHolderComponentShape::typeName );
	assert( s );
	
	MFnNumericAttribute nAttr;
	MFnTypedAttribute tAttr;
	
	aGLPreview = nAttr.create( "glPreview", "glpr", MFnNumericData::kBoolean, 1, &s );
	assert( s );
	nAttr.setReadable( true );
	nAttr.setWritable( true );
	nAttr.setStorable( true );
	nAttr.setConnectable( true );
	nAttr.setHidden( false );
	
	s = addAttribute( aGLPreview );
	assert( s );
	
	aTransparent = nAttr.create( "transparent", "trans", MFnNumericData::kBoolean, 0, &s );
	assert( s );
	nAttr.setReadable( true );
	nAttr.setWritable( true );
	nAttr.setStorable( true );
	nAttr.setConnectable( true );
	nAttr.setHidden( false );
	
	s = addAttribute( aTransparent );
	assert( s );
	
	aDrawBound = nAttr.create( "drawBound", "dbnd", MFnNumericData::kBoolean, 1, &s );
	assert( s );
	nAttr.setReadable( true );
	nAttr.setWritable( true );
	nAttr.setStorable( true );
	nAttr.setConnectable( true );
	nAttr.setHidden( false );
	
	s = addAttribute( aDrawBound );
	assert( s );
	
	IECoreGL::ConstStatePtr defaultState = IECoreGL::State::defaultState();
	assert( defaultState );
	assert( defaultState->isComplete() );
	MFnStringData fnData;
	MObject defaultValue = fnData.create( defaultState->get<const IECoreGL::NameStateComponent>()->name().c_str(), &s );
	assert( s );
	
	aProceduralComponents = tAttr.create( "proceduralComponents", "prcm", MFnData::kString, defaultValue, &s );
	assert( s );
	tAttr.setReadable( true );
	tAttr.setWritable( false );
	tAttr.setStorable( false );
	tAttr.setConnectable( true );
	tAttr.setHidden( true );
	tAttr.setArray( true );
	tAttr.setUsesArrayDataBuilder( true );	
	
	s = addAttribute( aProceduralComponents );
	assert( s );	
	
	return MS::kSuccess;
}

bool ProceduralHolder::isBounded() const
{
	return true;
}

MBoundingBox ProceduralHolder::boundingBox() const
{
	if( !m_boundDirty )
	{
		return m_bound;
	}
		
	m_bound = MBoundingBox( MPoint( -1, -1, -1 ), MPoint( 1, 1, 1 ) );

	Renderer::ProceduralPtr p = const_cast<ProceduralHolder*>(this)->getProcedural();
	if( p )
	{
		const_cast<ProceduralHolder*>(this)->setParameterisedValues();
		try
		{
			Box3f b = p->bound();
			if( !b.isEmpty() )
			{
				m_bound = convert<MBoundingBox>( b );
			}
		}
		catch( boost::python::error_already_set )
		{
			PyErr_Print();
		}
		catch( std::exception &e )
		{
			msg( Msg::Error, "ProceduralHolder::boundingBox", e.what() );
		}
		catch( ... )
		{
			msg( Msg::Error, "ProceduralHolder::boundingBox", "Exception thrown in Procedural::bound" );
		}
	}
	
	m_boundDirty = false;
	return m_bound;
}

MStatus ProceduralHolder::setDependentsDirty( const MPlug &plug, MPlugArray &plugArray )
{
	if( plugParameter( plug ) || (!plug.parent().isNull() && plugParameter( plug.parent() ) ) )
	{
		// it's an input to the procedural
		m_boundDirty = m_sceneDirty = true;
		childChanged( kBoundingBoxChanged ); // this is necessary to cause maya to redraw
	}
	
	return ParameterisedHolderComponentShape::setDependentsDirty( plug, plugArray );
}
				
MStatus ProceduralHolder::setProcedural( const std::string &className, int classVersion )
{
	return setParameterised( className, classVersion, "IECORE_PROCEDURAL_PATHS" );
}

IECore::Renderer::ProceduralPtr ProceduralHolder::getProcedural( std::string *className, int *classVersion )
{
	return runTimeCast<Renderer::Procedural>( getParameterised( className, classVersion ) );
}

IECoreGL::ConstScenePtr ProceduralHolder::scene()
{
	if( !m_sceneDirty  )
	{
		return m_scene;
	}
	
	m_scene = 0;
	Renderer::ProceduralPtr p = ((ProceduralHolder*)this)->getProcedural();
	if( p )
	{
		setParameterisedValues();
		try
		{
			IECoreGL::RendererPtr renderer = new IECoreGL::Renderer;
			renderer->setOption( "gl:mode", new StringData( "deferred" ) );
			renderer->worldBegin();
			
				p->render( renderer );
			
			renderer->worldEnd();
			
			m_scene = renderer->scene();
			m_scene->setCamera( 0 );
			
			buildComponents();
		}
		catch( boost::python::error_already_set )
		{
			PyErr_Print();
		}
		catch( IECore::Exception &e )
		{
			msg( Msg::Error, "ProceduralHolder::scene", e.what() );
		}
		catch( ... )
		{
			msg( Msg::Error, "ProceduralHolder::scene", "Exception thrown in Procedural::render" );
		}
	}
	
	m_sceneDirty = false;
	return m_scene;
}

void ProceduralHolder::componentToPlugs( MObject &component, MSelectionList &selectionList ) const
{
	MStatus s;
	
	if ( component.hasFn( MFn::kSingleIndexedComponent ) ) 
	{ 
		MFnSingleIndexedComponent fnComp( component, &s ); 
		assert( s );
		MObject thisNode = thisMObject(); 
		MPlug plug( thisNode, aProceduralComponents ); 
		assert( !plug.isNull() );
		
		int len = fnComp.elementCount( &s ); 
		assert( s );
		for ( int i = 0; i < len; i++ ) 
		{ 
			MPlug compPlug = plug.elementByLogicalIndex( fnComp.element(i), &s ); 
			assert( s );
			assert( !compPlug.isNull() );
			
			selectionList.add( compPlug ); 
		} 
	} 	
}

MPxSurfaceShape::MatchResult ProceduralHolder::matchComponent( const MSelectionList &item, const MAttributeSpecArray &spec, MSelectionList &list )
{
	if( spec.length() == 1 )
	{
		MAttributeSpec attrSpec = spec[0];
		MStatus s;

		int dim = attrSpec.dimensions();

		if ( (dim > 0) && (attrSpec.name() == "proceduralComponents" || attrSpec.name() == "prcm" || attrSpec.name() == "f" ) )
		{	
			int numComponents = componentToGroupMap().size();

			MAttributeIndex attrIndex = attrSpec[0];
			
			if ( attrIndex.type() != MAttributeIndex::kInteger )
			{
				return MPxSurfaceShape::kMatchInvalidAttributeRange;
			}
			
			int upper = numComponents - 1;
			int lower = 0;
			if ( attrIndex.hasLowerBound() )
			{
				attrIndex.getLower( lower );
			}
			if ( attrIndex.hasUpperBound() )
			{
				attrIndex.getUpper( upper );
			}

			// Check the attribute index range is valid
			if ( (attrIndex.hasRange() && !attrIndex.hasValidRange() ) || (upper >= numComponents) || (lower < 0 ) )
			{
				return MPxSurfaceShape::kMatchInvalidAttributeRange;		
			}

			MDagPath path;
			item.getDagPath( 0, path );
			MFnSingleIndexedComponent fnComp;
			MObject comp = fnComp.create( MFn::kMeshPolygonComponent, &s );
			assert( s );

			for ( int i=lower; i<=upper; i++ )
			{
				fnComp.addElement( i );
			}
			
			list.add( path, comp );

			return MPxSurfaceShape::kMatchOk;
		}	
	}

	return MPxSurfaceShape::matchComponent( item, spec, list );	
}

void ProceduralHolder::buildComponents( IECoreGL::ConstNameStateComponentPtr nameState, IECoreGL::GroupPtr group, MArrayDataBuilder &builder )
{	
	assert( nameState );
	assert( group );
	assert( group->getState() );				
	
	MStatus s;	
	
	IECoreGL::ConstNameStateComponentPtr ns = nameState;
	if (  group->getState()->get< IECoreGL::NameStateComponent >() )
	{
		ns = group->getState()->get< IECoreGL::NameStateComponent >();
	}

	const std::string &name = ns->name();		
	int compId = ns->glName();
	
	ComponentsMap::const_iterator it = componentsMap().find( name );
	if( it == componentsMap().end() )
	{		
		compId = componentsMap().size();
		componentsMap()[name] = compId;
		
		MFnStringData fnData;
		MObject data = fnData.create( MString( name.c_str() ) );
		
		MDataHandle h = builder.addElement( compId, &s );
		assert( s );

		s = h.set( data );
		assert( s );
	}
	
	componentToGroupMap()[compId].insert( ComponentToGroupMap::mapped_type::value_type( name, group ) );
		
	const IECoreGL::Group::ChildContainer &children = group->children();
	for ( IECoreGL::Group::ChildContainer::const_iterator it = children.begin(); it != children.end(); ++it )
	{
		assert( *it );
		
		IECoreGL::GroupPtr childGroup = runTimeCast< IECoreGL::Group >( *it );		
		if ( childGroup )
		{		
			buildComponents( ns, childGroup, builder );			
		}
	}
}

void ProceduralHolder::buildComponents()
{
	MStatus s;
	MDataBlock block = forceCache();
	
	MArrayDataHandle cH = block.outputArrayValue( aProceduralComponents, &s );
	assert( s );
	
	MArrayDataBuilder builder = cH.builder( &s );
	assert( s );

	componentsMap().clear();

	componentToGroupMap().clear();
	
	IECoreGL::ConstStatePtr defaultState = IECoreGL::State::defaultState();
	assert( defaultState );
	assert( defaultState->isComplete() );
		
	assert( m_scene );
	assert( m_scene->root() );	
	
	buildComponents( defaultState->get<const IECoreGL::NameStateComponent>(), m_scene->root(), builder );
	
	s = cH.set( builder );
	assert( s );
	
#ifndef NDEBUG
	MPlug plug( thisMObject(), aProceduralComponents );
	for ( ComponentsMap::const_iterator it = componentsMap().begin(); it != componentsMap().end(); ++it )
	{
		MPlug child = plug.elementByLogicalIndex( it->second, &s );
		assert( s );
		MObject obj;
		s = child.getValue( obj );
		assert( s );
		MFnStringData fnData( obj, &s );
		assert( s );
		
		assert( fnData.string() == MString( it->first.value().c_str() ) );
	}
	
#endif	
}
				

