#include "sop_groupasattr.h"

#include <GU/GU_Detail.h>
#include <OP/OP_Operator.h>
#include <GEO/GEO_AttributeHandle.h>
#include <OP/OP_AutoLockInputs.h>
#include <OP/OP_OperatorTable.h>
#include <PRM/PRM_Include.h>
#include <UT/UT_DSOVersion.h>
#include <GA/GA_Handle.h>
#include <UT/UT_Matrix3.h>
#include <UT/UT_Matrix4.h>
#include <SYS/SYS_Math.h>
#include <typeinfo>
#include <iostream>
#include <vector>
#include <stddef.h>

using namespace HDK_Sample;
using std::vector;
using namespace std;

void
newSopOperator(OP_OperatorTable *table)
{
    table->addOperator(new OP_Operator(
        "SOP_groupAsAttr",
        "Group As Attribute",
        SOP_groupAsAttr::myCtor,              //how to build a sop
        SOP_groupAsAttr::myTemplateList,      //mu ui parms
        1,                                  //min inputs
        1,                                  //max inputs
        0));                                //local vars
}

//set our attr lookup name
static PRM_Name names[] = {
    PRM_Name("getPointGrp", "Use Point Groups"),
    PRM_Name("getPrimGrp", "Use Prim Groups"),
//    PRM_Name("setCustomToggle", "Set attr name"),
//    PRM_Name("attrName", "Attribute Name"),
    PRM_Name("debug", "Debug console msgs"),
};

////static PRM_Conditional disAttr("{ useAttr == 1 }");
//static PRM_Conditional disAttr("{ setCustomToggle == 0 }");
//static PRM_Conditional disGrps("{ delPts == 1 }");
//static PRM_Conditional disDel("{ createGrp == 1 }");


PRM_Template
SOP_groupAsAttr::myTemplateList[] = {
    PRM_Template(PRM_STRING, 1, &PRMgroupName, 0,
    &SOP_Node::pointGroupMenu, 0, 0, SOP_Node::getGroupSelectButton(GA_GROUP_POINT)),

    PRM_Template(PRM_TOGGLE, 1, &names[0], PRMoneDefaults, 0, 0, 0, 0, 1, 0, 0),
    PRM_Template(PRM_TOGGLE, 1, &names[1],  PRMzeroDefaults, 0, 0, 0, 0, 1, 0, 0),
//    PRM_Template(PRM_TOGGLE, 1, &names[2], PRMzeroDefaults, 0, 0, 0, 0, 1, 0, 0),

//    PRM_Template(PRM_STRING, 1, &names[3], 0, 0, 0, 0, 0, 1, 0, &disAttr),
    PRM_Template(PRM_TOGGLE, 1, &names[2], PRMzeroDefaults),
    PRM_Template(),

//    PRM_Template(PRM_STRING, 1, &attrLookup,    0, &SOP_Node::pointAttribMenu, 0, 0, 0, 0, 0, &disP),
//    PRM_Template(PRM_TOGGLE, 1, &putInGrp,      PRMoneDefaults, 0, 0, 0, 0, 1, 0, &disGrps),
//    PRM_Template(PRM_STRING, 1, &newGroup,      0, 0, 0, 0, 0, 1, 0, &disGrps),
//    PRM_Template(PRM_TOGGLE, 1, &deletePts,     PRMzeroDefaults, 0, 0, 0, 0, 1, 0, &disDel),
////    PRM_Template(PRM_TOGGLE, 1, &comparePos,     PRMzeroDefaults, 0, 0, 0, 0, 1, 0, &disAttr),
//    PRM_Template(PRM_TOGGLE, 1, &debugMe,       PRMzeroDefaults),
};


OP_Node *
SOP_groupAsAttr::myCtor(OP_Network *net, const char *name, OP_Operator *op)
{
    return new SOP_groupAsAttr(net, name, op);
}

SOP_groupAsAttr::SOP_groupAsAttr(OP_Network *net, const char *name, OP_Operator *op)
    : SOP_Node(net, name, op), myGroup(NULL)
{
    // This indicates that this SOP manually manages its data IDs,
    // so that Houdini can identify what attributes may have changed,
    // e.g. to reduce work for the viewport, or other SOPs that
    // check whether data IDs have changed.
    // By default, (i.e. if this line weren't here), all data IDs
    // would be bumped after the SOP cook, to indicate that
    // everything might have changed.
    // If some data IDs don't get bumped properly, the viewport
    // may not update, or SOPs that check data IDs
    // may not cook correctly, so be *very* careful!
    mySopFlags.setManagesDataIDs(true);
//    primAttrCheck = false;
//    primAttrRename = false;
//    defaultName = "primGroupName";
}


SOP_groupAsAttr::~SOP_groupAsAttr() {}


OP_ERROR
SOP_groupAsAttr::cookInputGroups(OP_Context &context, int alone)
{
    // The SOP_Node::cookInputPointGroups() provides a good default
    // implementation for just handling a point selection.
    return cookInputPointGroups(
        context, // This is needed for cooking the group parameter, and cooking the input if alone.
        myGroup, // The group (or NULL) is written to myGroup if not alone.
        alone,   // This is true iff called outside of cookMySop to update handles.
                 // true means the group will be for the input geometry.
                 // false means the group will be for gdp (the working/output geometry).
        true,    // (default) true means to set the selection to the group if not alone and the highlight flag is on.
        0,       // (default) Parameter index of the group field
        -1,      // (default) Parameter index of the group type field (-1 since there isn't one)
        true,    // (default) true means that a pointer to an existing group is okay; false means group is always new.
        false,   // (default) false means new groups should be unordered; true means new groups should be ordered.
        true,    // (default) true means that all new groups should be detached, so not owned by the detail;
                 //           false means that new point and primitive groups on gdp will be owned by gdp.
        0        // (default) Index of the input whose geometry the group will be made for if alone.
    );
}

OP_ERROR
SOP_groupAsAttr::cookMySop(OP_Context &context)
{
    // We must lock our inputs before we try to access their geometry.
    // OP_AutoLockInputs will automatically unlock our inputs when we return.
    // NOTE: Don't call unlockInputs yourself when using this!
    OP_AutoLockInputs inputs(this);
    if (inputs.lock(context) >= UT_ERROR_ABORT)
        return error();

    // Duplicate our incoming geometry with the hint that we only
    // altered points.  Thus, if our input was unchanged, we can
    // easily roll back our changes by copying point values.
    duplicatePointSource(0, context);

    fpreal t = context.getTime();

    // We evaluate our parameters outside the loop for speed.  If we
    // wanted local variable support, we'd have to do more setup
    // (see SOP_Flatten) and also move these inside the loop.
//    UT_String attr_name;
    int get_PointGrp, get_PrimGrp;
//    int set_CustomToggle;

    get_PointGrp = getPointToggle(t);
    get_PrimGrp = getPrimToggle(t);
//    set_CustomToggle = setCustomNameToggle(t);
//    setAttrName(attr_name, t);

    debug = debugMe(t);

    cout << "debug      :" << debug << endl;
    cout << "get ptGrp  :" << get_PointGrp << endl;
    cout << "get ptimGrp:" << get_PrimGrp << endl;
//    cout << "attr name  :" << attr_name << endl;

    if (error() >= UT_ERROR_ABORT)
        return error();

    // Here we determine which groups we have to work on.  We only
    // handle point groups.
    if (cookInputGroups(context) >= UT_ERROR_ABORT)
    {
        return error();
    }


    //if we have chosen a point group
    if(get_PointGrp)
    {
        cout << "we are in points now!!" << endl;
        if(debug)
        {
            cout << "-------------------------------" <<endl;
            cout << "--*>YOU HAVE CHOSEN A POINT GROUP" << endl;
        }

//        //if we want a specific attr name
//        if(set_CustomToggle)
//        {
//            if(attr_name.length() > 0)
//            {
//                if(debug)
//                {
//                    cout << "---**>YOU HAVE CHOSEN TO RENAME THE DEFAULT ATTRIBUTE, THE ATTR NAME IS " << attr_name <<endl;
//                    cout << "-------------------------------------------------------------------------------------" << endl;
//                }

//                groupToAttrPoints(attr_name, debug);
//            }
//            else
//            {
//                if(debug)
//                {
//                    cout << "---**>YOU HAVE CHOSEN TO RENAME THE DEFAULT ATTRIBUTE, THE ATTR NAME IS BLANK" <<endl;
//                    cout << "---****>PLEASE ENTER A NAME FOR THE NEW ATTR" << endl;
//                    cout << "-------------------------------------------------------------------------------------" << endl;
//                }
//            }

//        }
//        else
//        {
//            if(debug)
//            {
//                cout << "---**>YOU HAVE CHOSEN NOT TO RENAME THE DEFAULT ATTRIBUTE, THE ATTR NAME IS ptGroupName" <<endl;
//                cout << "-------------------------------------------------------------------------------------" << endl;
//            }

            groupToAttrPoints("ptGroupName", debug);
//        }
    }

    //IF WE HAVE CHOSEN TO DEAL WITH PRIMITIVES
    if(get_PrimGrp)
    {
        //set our flag to signal that we have chosen to use the prim attr
        if(debug)
        {
            cout << "-------------------------------" <<endl;
            cout << "--*>YOU HAVE CHOSEN A PRIM GROUP" << endl;
        }

//        //if we want a specific attr name
//        if(set_CustomToggle)
//        {
//            cout << endl << "in set custom toggle now and OldName is " << oldName << endl;

//            if(attr_name.length() > 0)
//            {
//                cout << endl << "attr name is entered now and OldName is " << oldName << endl;

//                if(debug)
//                {
//                    cout << "---**>YOU HAVE CHOSEN TO RENAME THE DEFAULT ATTRIBUTE, THE ATTR NAME IS " << attr_name <<endl;
//                    cout << "-------------------------------------------------------------------------------------" << endl;
//                }

//                if(!primAttrRename)
//                {
//                    gdp->renameAttribute(GA_ATTRIB_PRIMITIVE,GA_SCOPE_PUBLIC, defaultName, attr_name);
//                    oldName = attr_name;
//                    primAttrRename = true;
//                    cout << "This is the first time you are renaming attrs from " << defaultName << " to " << attr_name << endl;
//                    cout << "oldName is " << oldName << endl;
//                }
//                else
//                {
//                    cout << "This is NOT first time you are renaming attrs from " << oldName << " to " << attr_name << endl;
//                    cout << "oldName is " << oldName << endl;
//                    gdp->renameAttribute(GA_ATTRIB_PRIMITIVE,GA_SCOPE_PUBLIC, oldName, attr_name);
//                    oldName = attr_name;
//                    cout << "new oldName is " << oldName << endl;

//                }

////                primAttrRename = true;
////                groupToAttrPrims(attr_name, set_CustomToggle, debug);
//            }
//            else
//            {
//                cout << "nothing in the attr text field" << endl;
//                if(debug)
//                {
//                    cout << "---**>YOU HAVE CHOSEN TO RENAME THE DEFAULT ATTRIBUTE, THE ATTR NAME IS BLANK" <<endl;
//                    cout << "---****>PLEASE ENTER A NAME FOR THE NEW ATTR" << endl;
//                    cout << "-------------------------------------------------------------------------------------" << endl;
//                }
//            }

//        }
//        else
//        {
//            if(debug)
//            {
//                cout << "---**>YOU HAVE CHOSEN NOT TO RENAME THE DEFAULT ATTRIBUTE, THE ATTR NAME IS ptGroupName" <<endl;
//                cout << "-------------------------------------------------------------------------------------" << endl;
//            }

//            if(primAttrRename)
//            {
//                cout << "you have truned off the custom attr name and the attr " << attr_name << "will be renamed into " << defaultName << endl;
//                gdp->renameAttribute(GA_ATTRIB_PRIMITIVE,GA_SCOPE_PUBLIC, attr_name, defaultName);
//                primAttrRename = false;
//            }
//            else
//            {
//                cout << "you have NEVER HAD custom attr name and the attr " << attr_name << "will be renamed into " << defaultName << endl;
                groupToAttrPrims("primGroupName", debug);
//            }
//        }
    }
    else
    {

//        if(primAttrCheck)
//        {

//            if(set_CustomToggle)
//            {
//                if(primAttrRename)
//                {
//                    //destroy the attr
//                    gdp->destroyAttribute(GA_ATTRIB_PRIMITIVE, attr_name);
//                    primAttrCheck = false;
//                    primAttrRename = false;
//                }
//                else
//                {
//                    //destroy the attr
                    gdp->destroyAttribute(GA_ATTRIB_PRIMITIVE, "primGroupName");
//                    primAttrCheck = false;
//                    primAttrRename = false;
//                }
//            }
//            else
//            {
//                if(primAttrRename)
//                {
//                    gdp->destroyAttribute(GA_ATTRIB_PRIMITIVE, attr_name);
//                    primAttrRename = false;
//                    primAttrCheck = false;
//                }
//                else
//                {
//                    gdp->destroyAttribute(GA_ATTRIB_PRIMITIVE, defaultName);
//                    primAttrCheck = false;
//                }
//            }

//        }

    }

    //    const GA_ElementGroupTable & grpTable = gdp->getElementGroupTable(GA_ATTRIB_POINT);

    //    for (GA_GroupTable::iterator<GA_ElementGroup> it=grpTable.beginTraverse(); !it.atEnd(); ++it)
    //    {
    //        GA_PointGroup *group = static_cast<GA_PointGroup*>( it.group() );
    //        cout << "groups?" << group->getName() << endl;
    //    }

    // If we've modified P, and we're managing our own data IDs,
    // we must bump the data ID for P.
    if (!myGroup || !myGroup->isEmpty())
        gdp->bumpAllDataIds();
//        gdp->getP()->bumpDataId();

    return error();
}

void SOP_groupAsAttr::groupToAttrPrims(const UT_String& in_attr, const int& debug_in)
{
    //find if a pt attr with same in_attr name already exist, if not create it
//    GA_ROHandleS prmattr(gdp, GA_ATTRIB_PRIMITIVE, in_attr);

//    GA_WOAttributeRef prmattr(gdp->findStringTuple(GA_ATTRIB_PRIMITIVE, in_attr));


        groupNameIndex = gdp->findStringTuple(GA_ATTRIB_PRIMITIVE, in_attr);

        if(groupNameIndex.isInvalid())
        {
            groupNameIndex = gdp->addStringTuple(GA_ATTRIB_PRIMITIVE, in_attr, 1);
            if(groupNameIndex.isInvalid())
            {
                addError(SOP_ATTRIBUTE_INVALID, "Unable to create primitive attribut");
    //            return error();
            }
        }


////        prmattr = GA_WOAttributeRef (gdp->addStringTuple(GA_ATTRIB_PRIMITIVE, in_attr, 1));

////        GA_Attribute *atr = gdp->createStringAttribute(GA_ATTRIB_PRIMITIVE,in_attr);
////        GA_Attribute *atr = gdp->addPrimAttrib(prmattr);


//        //find the default attr
////        prmattr = GA_RWHandleS(gdp->addStringTuple(GA_ATTRIB_PRIMITIVE,in_attr,1));

//        //iterate thru all the points and for every point iterate thru all the groups
        GA_Primitive *prim;
        GA_FOR_ALL_PRIMITIVES(gdp, prim)
        {

////            GA_RWAttributeRef h = gdp->addStringTuple(GA_ATTRIB_PRIMITIVE, in_attr, 1);
            bool exist = false;

            //set our current group
            GA_PrimitiveGroup *curGrp;
            //for all the point groups
            GA_FOR_ALL_PRIMGROUPS(gdp, curGrp)
            {
                if(curGrp)
                {
                    if(exist == false)
                    {
                        if(curGrp->contains(*prim))
                        {
                            prim->setString(groupNameIndex, curGrp->getName());
//                            int idx = prim->getStringHandle(h);
//                            prmattr.set((GA_Offset)prim->getNum(), curGrp->getName());
//                            prim->setString(h, "testing");
//                            if(debug_in)
//                                cout << "===>PRIM " << prim->getNum() << " IS IN GROUP " << curGrp->getName() << endl;
                            exist = true;
                        }
                        else
                        {
                            prim->setString(groupNameIndex, "no group");
//                            int idx = prim->getStringHandle(h);
//                            stratt.set(prim->getNum(), "no group");
                        }
                    }
                }
            }
        }
        GA_Attribute *atr = groupNameIndex.get();
        atr->bumpDataId();
    }
//    else
//    {
//        if(debug)
//            cout << "==>ATTRIBUTE ALREADY EXISTS, TRY AND USE A CUSTOM ATTR FROM ATTR NAME!" << endl;
//    }
//}

//our method for setting the group names as attr on points
void SOP_groupAsAttr::groupToAttrPoints(const UT_String& in_attr, const int& debug_in)
{
    //find if a pt attr with same in_attr name already exist, if not create it
    GA_RWHandleS stratt(gdp->findStringTuple(GA_ATTRIB_POINT, in_attr));

    //if the attribute with this name does not exist
    if(!stratt.isValid())
    {
        //debug
        if(debug_in)
        {
            cout << "*>THE ATTRIBUTE DOES NOT EXIST, CREATED THE ATTRIB" << endl;
        }

        //find the default attr
        stratt = GA_RWHandleS(gdp->addStringTuple(GA_ATTRIB_POINT,in_attr,1));

        //iterate thru all the points and for every point iterate thru all the groups
        GA_Offset offset;
        GA_FOR_ALL_PTOFF(gdp, offset)
        {
            bool exist = false;

            //set our current group
            GA_PointGroup *curGrp;
            //for all the point groups
            GA_FOR_ALL_POINTGROUPS(gdp, curGrp)
            {
                if(curGrp)
                {
                    if(exist == false){
                        if(curGrp->containsOffset(offset))
                        {
                            stratt.set(offset, curGrp->getName());
                            if(debug_in)
                                cout << "===>POINT " << offset << " IS IN GROUP " << curGrp->getName() << endl;
                            exist = true;
                        }
                        else
                        {
                            stratt.set(offset, "no group");
                        }
                    }
                }
            }
        }
//        stratt.bumpDataId();
    }
    else
    {
        cout << "==>ATTRIBUTE ALREADY EXISTS, TRY AND USE A CUSTOM ATTR FROM ATTR NAME!" << endl;
    }
}
