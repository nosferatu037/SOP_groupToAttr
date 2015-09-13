#ifndef SOP_GROUPASATTR_H
#define SOP_GROUPASATTR_H


/*
 * File:   SOP_groupAsAttr.h
 * Author: nosferatu
 *
 * Created on August 29, 2015, 1:22 PM
 */


#include <SOP/SOP_Node.h>

namespace HDK_Sample
{

    class SOP_groupAsAttr : public SOP_Node
    {
    public:
        SOP_groupAsAttr(OP_Network *net, const char *name, OP_Operator *op);
        virtual ~SOP_groupAsAttr();


        static PRM_Template myTemplateList[];
        static OP_Node  *myCtor(OP_Network*, const char*, OP_Operator*);
        /// This method is created so that it can be called by handles.  It only
        /// cooks the input group of this SOP.  The geometry in this group is
        /// the only geometry manipulated by this SOP.
        virtual OP_ERROR    cookInputGroups(OP_Context &context, int alone = 0);

    protected:
        /// Method to cook geometry for the SOP
        virtual OP_ERROR    cookMySop(OP_Context &context);

    private:


        //get our group to be point group
        int    getPointToggle(fpreal t)
        {
            return evalInt("getPointGrp", 0, t);
        }
        //get our group to be prim group
        int    getPrimToggle(fpreal t)
        {
            return evalInt("getPrimGrp", 0, t);
        }

        //toggle debug
        int debugMe(fpreal t)
        {
            return evalInt("debug", 0, t);
        }


        GA_RWAttributeRef groupNumIndex, groupNameIndex;

        void groupToAttrPoints(const UT_String& in_attr, const int& debug_in);
        void groupToAttrPrims(const UT_String& in_attr, const int& debug_in);

        int debug;

//        /// This variable is used together with the call to the "checkInputChanged"
//        /// routine to notify the handles (if any) if the input has changed.
//        GU_DetailGroupPair	 myDetailGroupPair;

        /// This is the group of geometry to be manipulated by this SOP and cooked
        /// by the method "cookInputGroups".
        const GA_PointGroup	*myGroup;

    };
}

#endif // SOP_GROUPASATTR_H
