//----------------------------------------------------------------------------
//
// License:  LGPL
// 
// See LICENSE.txt file in the top level directory for more details.
//
// Author:  David Burken
//
// Description: HDF5 Info class.
// 
//----------------------------------------------------------------------------
// $Id$

#include <ossim/hdf5/ossimHdf5Info.h>
#include <ossim/base/ossimCommon.h>
#include <ossim/base/ossimEndian.h>
#include <ossim/base/ossimNotify.h>
#include <ossim/base/ossimTrace.h>
#include <ossim/base/ossimScalarTypeLut.h>
#include <ossim/hdf5/ossimHdf5.h>

#include <fstream>
#include <iostream>
#include <iomanip>
#include <sstream>

using namespace std;
using namespace H5;

ossimHdf5Info::ossimHdf5Info()
: ossimInfoBase()
{
}

ossimHdf5Info::ossimHdf5Info(ossimHdf5* hdf5)
: ossimInfoBase(),
  m_hdf5 (hdf5)
{
}

ossimHdf5Info::~ossimHdf5Info()
{
   m_hdf5 = 0;
}

bool ossimHdf5Info::open(const ossimFilename& file)
{
   m_hdf5 = new ossimHdf5;
   if (!m_hdf5->open(file))
   {
      m_hdf5 = 0;
      return false;
   }
   return true;
}

// Top level print from root
ostream& ossimHdf5Info::print(ostream& out) const
{
   if (!m_hdf5.valid())
   {
      out<<"ossimHdf5Info: No HDF5 file has been opened! Nothing to print."<<endl;
      return out;
   }

   try
   {
      Group root;
      if (!m_hdf5->getRoot(root))
         return out;
      print(out, root, "");
      out<<endl;
   }
   catch (H5::Exception& h5x)
   {
      h5x.getDetailMsg();
   }

   return out;
}

// GROUP LIST
ostream& ossimHdf5Info::printSubGroups(std::ostream& out, const H5::Group& group,
                                       const ossimString& lm) const
{
   vector<Group> groups;
   if (!m_hdf5->getChildGroups(group, groups))
      return out;
   if (!groups.empty())
   {
      for (ossim_uint32 i=0; i<groups.size(); ++i)
         print(out, groups[i], lm);
   }
   return out;
}

// ATTRIBUTE LIST
ostream& ossimHdf5Info::printAttributes(std::ostream& out, const H5::H5Object& obj,
                                        const ossimString& lm) const
{
   vector<Attribute> attributes;
   if (!m_hdf5->getAttributes(obj, attributes))
      return out;

   if (!attributes.empty())
   {
      for (ossim_uint32 i=0; i<attributes.size(); ++i)
         print(out, attributes[i], lm);
   }
   return out;
}

// DATASET LIST
ostream& ossimHdf5Info::printDatasets(std::ostream& out, const H5::Group& group,
                                      const ossimString& lm) const
{
   vector<DataSet> datasets;
   if (!m_hdf5->getDatasets(group, datasets))
      return out;

   if (!datasets.empty())
   {
      for (ossim_uint32 i=0; i<datasets.size(); ++i)
         print(out, datasets[i], lm);
   }
   return out;
}

// GROUP
ostream& ossimHdf5Info::print(ostream& out, const H5::Group& group, const ossimString& lm) const
{
   try
   {
      out<<lm<<"GROUP: "<<group.getObjName()<<endl;

      // Set indent for children:
      ossimString lm2 (lm + "  ");

      // List attributes:
      printAttributes(out, group, lm2);

      // List Datasets:
      printDatasets(out, group, lm2);

      // List Child Groups:
      printSubGroups(out, group, lm2);
   }
   catch( const H5::Exception& e )
   {
      ossimNotify(ossimNotifyLevel_WARN)<<e.getDetailMsg();
   }

   return out;
}

// DATASET
ostream& ossimHdf5Info::print(ostream& out, const H5::DataSet& dataset, const ossimString& lm) const
{
   try
   {
      out<<lm<<"DATASET: "<<dataset.getObjName()<<endl;

      // Dump its components:
      int set_size = dataset.getSpace().getSimpleExtentNpoints();
      ossimString lm2 (lm + "  ");
      print(out, dataset.getDataType(), lm2);
      print(out, dataset.getSpace(), lm2);

      // Dump dataset values for small datasets:
      H5T_class_t class_type = dataset.getDataType().getClass();
      if ((set_size < 11) && (class_type == H5T_STRING))
      {
         string values;
         dataset.read(values, dataset.getDataType());
         out<<lm<<"  values: "<<values<<endl;
      }
   }
   catch( const H5::Exception& e )
   {
      ossimNotify(ossimNotifyLevel_WARN)<<e.getDetailMsg();
   }

   return out;
}

// DATATYPE
ostream& ossimHdf5Info::print(ostream& out, const H5::DataType& datatype, const ossimString& lm) const
{
   try
   {
      H5T_class_t class_type = datatype.getClass();
      size_t size = datatype.getSize();
      bool isAtomic = false;
      switch (class_type)
      {
      case H5T_INTEGER:
         isAtomic = true;
         out<<lm<<"DATATYPE: integer, "<<size<<" bytes ";
         break;
      case H5T_FLOAT:
         isAtomic = true;
         out<<lm<<"DATATYPE: float, "<<size<<" bytes ";
         break;
      case H5T_TIME:
         out<<lm<<"DATATYPE: date/time, "<<size<<" bytes "<<endl;
         break;
      case H5T_STRING:
         out<<lm<<"DATATYPE: string, "<<size<<" bytes ";
         break;
      case H5T_BITFIELD:
         out<<lm<<"DATATYPE: bit-field, "<<size<<" bytes "<<endl;
         break;
      case H5T_OPAQUE:
         out<<lm<<"DATATYPE: opaque, "<<size<<" bytes "<<endl;
         break;
      case H5T_COMPOUND:
         out<<lm<<"DATATYPE: compound, "<<size<<" bytes "<<endl;
         break;
      case H5T_REFERENCE:
         out<<lm<<"DATATYPE: reference, "<<size<<" bytes "<<endl;
         break;
      case H5T_ENUM:
         out<<lm<<"DATATYPE: enumeration, "<<size<<" bytes "<<endl;
         break;
      case H5T_VLEN:
         out<<lm<<"DATATYPE: variable-length, "<<size<<" bytes "<<endl;
         break;
      case H5T_ARRAY:
         out<<lm<<"DATATYPE: array, "<<size<<" bytes "<<endl;
         break;
      default:
         out<<lm<<"DATATYPE: unknown, "<<size<<" bytes "<<endl;
      }

      if (isAtomic)
      {
         H5T_order_t order = ((AtomType&) datatype).getOrder();
         switch (order)
         {
         case H5T_ORDER_LE:
            out<<"(Little Endian)"<<endl;
            break;
         case H5T_ORDER_BE:
            out<<"(Big Endian)"<<endl;
            break;
         default:
            out<<endl;
            break;
         }
      }
   }
   catch( const H5::Exception& e )
   {
      ossimNotify(ossimNotifyLevel_WARN)<<e.getDetailMsg();
   }


   return out;
}

// DATASPACE
ostream& ossimHdf5Info::print(ostream& out, const H5::DataSpace& dataspace, const ossimString& lm) const
{
   int rank = 0;
   hsize_t* dim_sizes = 0;

   try
   {
      H5S_class_t classT = dataspace.getSimpleExtentType();
      switch (classT)
      {
      case H5S_SCALAR:
         out<<lm<<"DATASPACE: (scalar)"<<endl;
         break;
      case H5S_SIMPLE:
         rank = dataspace.getSimpleExtentNdims();
         dim_sizes = new hsize_t[rank];
         dataspace.getSimpleExtentDims(dim_sizes);
         out<<lm<<"DATASPACE: simple, rank: "<<rank<<"  size: ";
         for (int i=0; i<rank; i++)
         {
            int dim_size = dim_sizes[i];
            if (i > 0)
               out<<" x ";
            out<<dim_size;
         }
         out<<endl;
         delete dim_sizes;
         break;
      case H5S_NULL:
         out<<lm<<"DATASPACE: (NULL)"<<endl;
         break;
      default:
         out<<lm<<"DATASPACE: (Unknown Type)"<<endl;
      }
   }
   catch( const H5::Exception& e )
   {
      ossimNotify(ossimNotifyLevel_WARN)<<e.getDetailMsg();
   }

   return out;
}

// ATTRIBUTE
ostream& ossimHdf5Info::print(ostream& out, const H5::Attribute& attr, const ossimString& lm) const
{
   out<<lm<<"ATTRIBUTE: "<<attr.getName();

   try
   {
      std::string str_value;
      char buf[1024];
      //int int_value = 0;
      ossimString lm2 (lm + "  ");
      ossimByteOrder order = m_hdf5->getByteOrder(&attr);
      ossimEndian endian;
      bool swapOrder = (order!=ossim::byteOrder());

      H5T_class_t class_type = attr.getDataType().getClass();
      ossim_uint32 dataSize = attr.getDataType().getSize();
      switch (class_type)
      {
      case H5T_STRING:
         attr.read(attr.getDataType(), str_value);
         out <<" = "<<str_value<<endl;
         break;
      case H5T_INTEGER:
      {
         std::string strValue;
         attr.read(attr.getDataType(), buf);
         ossim_uint32 signType = H5Tget_sign(attr.getDataType().getId());
         switch(dataSize)
         {
            case 1: // one byte integer
            {
               switch(signType)
               {
                  case H5T_SGN_NONE:
                  {
                    ossim_uint8* intValue=0;
                    intValue = reinterpret_cast<ossim_uint8*>(buf);
                    strValue = ossimString::toString(*intValue).string();

                     break;
                  }
                  case H5T_SGN_2:
                  {
                    ossim_int8* intValue=0;
                    intValue = reinterpret_cast<ossim_int8*>(buf);
                    strValue = ossimString::toString(*intValue).string();

                    break;
                  }
                  default:
                  {
                     break;
                  }
               }
               break;
            }
            case 2:  // 2 byte integer
            {
               switch(signType)
               {
                  case H5T_SGN_NONE: // unsigned
                  {
                    ossim_uint16* intValue=0;
                    intValue = reinterpret_cast<ossim_uint16*>(buf);
                    if (swapOrder)
                       endian.swap(*intValue);
                    strValue = ossimString::toString(*intValue).string();

                     break;
                  }
                  case H5T_SGN_2: // Signed
                  {
                    ossim_int16* intValue=0;
                    intValue = reinterpret_cast<ossim_int16*>(buf);
                    if (swapOrder)
                       endian.swap(*intValue);
                    strValue = ossimString::toString(*intValue).string();
                    break;
                  }
                  default:
                  {
                     break;
                  }

               }
               break;
            }
            case 4: // 4 byte integer
            {
               switch(signType)
               {
                  case H5T_SGN_NONE:
                  {
                    ossim_uint32* intValue=0;
                    intValue = reinterpret_cast<ossim_uint32*>(buf);
                    if (swapOrder)
                       endian.swap(*intValue);
                    strValue = ossimString::toString(*intValue).string();

                     break;
                  }
                  case H5T_SGN_2:
                  {
                    ossim_int32* intValue=0;
                    intValue = reinterpret_cast<ossim_int32*>(buf);
                    if (swapOrder)
                       endian.swap(*intValue);
                    strValue = ossimString::toString(*intValue).string();
                    break;
                  }
                  default:
                  {
                     break;
                  }
               }
               break;
            }
            case 8: // 8 byte integer
            {
               switch(signType)
               {
                  case H5T_SGN_NONE:
                  {
                    ossim_uint64* intValue=0;
                    intValue = reinterpret_cast<ossim_uint64*>(buf);
                    if (swapOrder)
                       endian.swap(*intValue);
                    strValue = ossimString::toString(*intValue).string();

                     break;
                  }
                  case H5T_SGN_2:
                  {
                    ossim_int64* intValue=0;
                    intValue = reinterpret_cast<ossim_int64*>(buf);
                    if (swapOrder)
                       endian.swap(*intValue);
                    strValue = ossimString::toString(*intValue).string();
                    break;
                  }
               }
               break;
            }

         }
         out <<" = "<<strValue<<endl;
         break;
      }
      case H5T_FLOAT:
      {
         std::string strValue;
         attr.read(attr.getDataType(), buf);
         switch(dataSize)
         {
            // we will use a buf pointer.  There is something going on with some datasets
            // and the attribute reader core dumping when providing the address of a float
            // To fix this we will use a char* buf and reinterpret the cast.
            case 4:
            {
               ossim_float32* float_value=0;
               float_value = reinterpret_cast<ossim_float32*>(buf);
               if (swapOrder)
                  endian.swap(*float_value);
               strValue = ossimString::toString(*float_value).string();
               break;
            }
            case 8:
            {
               ossim_float64* float_value=0;
               float_value = reinterpret_cast<ossim_float64*>(buf);
               if (swapOrder)
                  endian.swap(*float_value);
               strValue = ossimString::toString(*float_value).string();
               break;
            }
         }
         out <<" = "<<strValue<<endl;
         break;
      }
      default:
         out <<" (value not handled type) "<<endl;
         print(out, attr.getDataType(), lm2);
         print(out, attr.getSpace(), lm2);
      }
   }
   catch( const H5::Exception& e )
   {
      ossimNotify(ossimNotifyLevel_WARN)<<e.getDetailMsg();
   }

   return out;
}

bool ossimHdf5Info::getKeywordlist(ossimKeywordlist& kwl) const
{
   m_kwl.clear();

   try
   {
      if (!m_hdf5.valid())
         return false;

      string groupName     = "/";
      string prefix        = "hdf5.";

      Group root;
      if (!m_hdf5->getRoot(root))
         return false;

      ossim_uint32 recurseCount = 0;
      dumpGroup(root, prefix, recurseCount);

      // Dump daset names:
      vector<DataSet> datasets;
      vector<std::string> datasetNames;
      m_hdf5->getDatasets(root, datasets, true );
      ostringstream value;
      value << "(";
      for (ossim_uint32 i=0; i<datasets.size(); ++i)
      {
         if (i == 0)
            value << datasets[i].getObjName();
         else
            value << ", "<< datasets[i].getObjName();
      }
      value<<")";
      m_kwl.addPair(prefix, string("datasetnames"), value.str());
   }
   catch( const H5::Exception& e )
   {
      ossimNotify(ossimNotifyLevel_WARN)<<e.getDetailMsg();
   }


   kwl = m_kwl;
   return true;
}

bool ossimHdf5Info::getKeywordlistDataset(ossimKeywordlist& kwl, const std::string& datasetName) const
{
  bool returnValue = false;
  m_kwl.clear();
  Group root;
  if (m_hdf5->getRoot(root))
  {
     H5::DataSet* dataset = m_hdf5->findDatasetByName(datasetName, &root, true);
     if(dataset)
     {
         try{
            dumpDataset(*dataset, "");
            returnValue = true;
            kwl = m_kwl;

         }
         catch(...)
         {

         }
        delete dataset;
     }
  }
  return returnValue; 
}

bool ossimHdf5Info::getKeywordlistGroup(ossimKeywordlist& kwl, const std::string& groupName) const
{
  bool returnValue = false;
  m_kwl.clear();
  Group root;
  ossim_uint32 recurseCount = 0;

  if (m_hdf5->getRoot(root))
  {
     H5::Group* group = m_hdf5->findGroupByName(groupName, &root, true);
     if(group)
     {
         try{
            dumpGroup(*group, "", recurseCount);
            returnValue = true;
            kwl = m_kwl;

         }
         catch(...)
         {

         }
        delete group;
     }
  }
  return returnValue; 
}

void ossimHdf5Info::dumpGroup(const Group& group,
                              const string& prefix,
                              ossim_uint32& recursedCount) const
{
   ++recursedCount;

   try
   {
      ossimString groupPrefix = getObjectPrefix(prefix, group.getObjName());
      m_kwl.addPair(groupPrefix, string("type"), string("Group"));

      // Dump all attributes for this group:
      dumpAttributes(group, groupPrefix);

      // Dump all datasets under this group:
      vector<DataSet> datasets;
      m_hdf5->getDatasets(group, datasets);
      for (ossim_uint32 i=0; i<datasets.size(); ++i)
      {
         dumpDataset(datasets[i], groupPrefix);
      }

      // Dump child Groups:
      vector<Group> childGroups;
      m_hdf5->getChildGroups(group, childGroups, false);
      for (ossim_uint32 i=0; i<childGroups.size(); ++i)
      {
         dumpGroup(childGroups[i], groupPrefix, recursedCount);
      }
   }
   catch( const H5::Exception& e )
   {
      ossimNotify(ossimNotifyLevel_WARN)<<e.getDetailMsg();
   }

   --recursedCount;
}

ossimString ossimHdf5Info::getObjectPrefix(const ossimString& prefix,
                                           const ossimString& fullPathName) const
{
   vector<ossimString> items;
   fullPathName.split(items, "/");
   ossimString objectName (items.back());

   ostringstream objectPrefix;
   if (objectName.empty())
      objectPrefix << prefix;
   else
      objectPrefix << prefix << objectName<<".";

   return objectPrefix.str();
}

void ossimHdf5Info::dumpAttributes(const H5Object& obj, const std::string& prefix) const
{
   try
   {
      vector<H5::Attribute> attrList;
      m_hdf5->getAttributes(obj, attrList);
      for ( ossim_uint32 i = 0; i < attrList.size(); ++i )
      {
         ostringstream attrPrefix;
         attrPrefix << prefix;//<<"attribute"<<i<<".";
         dumpAttribute( attrList[i], attrPrefix.str());
      }
   }
   catch( const H5::Exception& e )
   {
      ossimNotify(ossimNotifyLevel_WARN)<<e.getDetailMsg();
   }
}

void ossimHdf5Info::dumpAttribute(const H5::Attribute& attr,
                                  const std::string& prefix) const
{
   std::string strValue;
   char buf[1024];
   try
   {
      ossimByteOrder order = m_hdf5->getByteOrder(&attr);
      ossimEndian endian;
      // bool swapOrder = (order!=ossim::byteOrder());

      H5T_class_t class_type = attr.getDataType().getClass();
      // ossim_uint32 dataSize = attr.getDataType().getSize();
      switch (class_type)
      {
         case H5T_STRING:
         {
            attr.read(attr.getDataType(), strValue);
            break;
         }
         case H5T_INTEGER:
         {
            H5::DataType  dataType = attr.getDataType();
            H5::IntType*  dataTypePtr = (H5::IntType*)(&dataType);

            attr.read(attr.getDataType(), buf);
            ossimHdf5::intTypeToString(strValue, *dataTypePtr, buf);
            break;
         }
         case H5T_FLOAT:
         {
            H5::DataType  dataType = attr.getDataType();
            H5::FloatType*  dataTypePtr = (H5::FloatType*)(&dataType);

            attr.read(attr.getDataType(), buf);
            ossimHdf5::floatTypeToString(strValue, *dataTypePtr, buf);
            break;
         }
         case H5T_COMPOUND:
         {
            //H5::DataType  dataType = attr.getDataType();
            //H5::FloatType*  dataTypePtr = (H5::FloatType*)(&dataType);

            //H5::CompType compType(dataset);
            //dumpCompoundTypeInfo(compType, datasetPrefix);
            //dumpCompound(dataset, compType, datasetPrefix);
            // ostringstream tempOut;

            // tempOut << "(" <<"H5T_COMPOUND not a handled type)";
            // strValue = tempOut.str();
           
         }
         default:
         {
            ostringstream tempOut;

            tempOut << "(" << ossimHdf5::getDatatypeClassType(class_type) <<" not a handled type)";
            strValue = tempOut.str();
         }
      }

      ossimString attrKey (getObjectPrefix(prefix, attr.getName()));
      m_kwl.addPair(prefix, attr.getName(), strValue);
   }
   catch( const H5::Exception& e )
   {
      ossimNotify(ossimNotifyLevel_WARN)<<e.getDetailMsg();
   }
}


void ossimHdf5Info::dumpDataset(const H5::DataSet& dataset,
                                const std::string& prefix) const
{
#if 0
   std::cout << "printObject entered..."
         << "\nobjectName: " << objectName
         << "\nprefix: " << prefix
         << std::endl;
#endif

   try
   {
      string datasetPrefix = getObjectPrefix(prefix, dataset.getObjName());
      m_kwl.addPair(datasetPrefix, string("type"), string("DataSet"));

      // Dump all attributes for this dataset:
      dumpAttributes(dataset, datasetPrefix);

      // Get the class of the datatype that is used by the dataset.
      H5T_class_t type_class = dataset.getTypeClass();
      m_kwl.addPair(datasetPrefix, "class_type", m_hdf5->getDatatypeClassType(type_class));

      // Dump specific datatypes:
      switch(type_class)
      {
         case H5T_COMPOUND:
         {
            H5::CompType compType(dataset);
            dumpCompoundTypeInfo(compType, datasetPrefix);
            dumpCompound(dataset, compType, datasetPrefix);
            break;
         }
         case H5T_ENUM:
         {
           H5::EnumType enumType (dataset);
           dumpEnumTypeInfo(enumType, datasetPrefix);
            break;
         }
         case H5T_ARRAY:
         {
            H5::ArrayType arrayType (dataset.getId());
            dumpArrayTypeInfo(arrayType, datasetPrefix);
            break;
         }
         case H5T_INTEGER:
         case H5T_FLOAT:
         {
            ossimByteOrder byteOrder = m_hdf5->getByteOrder( &dataset );
            dumpNumericalTypeInfo(dataset, byteOrder, datasetPrefix);
            break;
         }
         default:
         {
           m_kwl.addPair(datasetPrefix, string(ossimKeywordNames::SCALAR_TYPE_KW),
                         string("OSSIM_SCALAR_UNKNOWN"));
            break;
         }
      }

//      Dump Extents:
      vector<ossim_uint32> extents;
      m_hdf5->getExtents( dataset, extents );
      ostringstream value;
      if(!extents.empty())
      {
         value <<extents[0];
         for ( ossim_uint32 i = 1; i < extents.size(); ++i )
         {
            value << ", " << extents[i];
         }
         m_kwl.addPair(datasetPrefix, "extents", value.str());
      }

#if 0
      // Attributes:
      int numberOfAttrs = dataset.getNumAttrs();
      cout << "numberOfAttrs: " << numberOfAttrs << endl;
      for ( ossim_int32 attrIdx = 0; attrIdx < numberOfAttrs; ++attrIdx )
      {
         H5::Attribute attribute = dataset.openAttribute( attrIdx );
         cout << "attribute.from class: " << attribute.fromClass() << endl;
      }
#endif
   }
   catch( const H5::Exception& e )
   {
      ossimNotify(ossimNotifyLevel_WARN)<<e.getDetailMsg();
   }

}

void ossimHdf5Info::dumpCompound(const H5::DataSet& dataset,
                                 const H5::CompType& compound,
                                 const std::string& prefix)const
{
   H5::DataSpace dataspace = dataset.getSpace();
   ossim_uint32 dimensions = dataspace.getSimpleExtentNdims();
   ossim_uint32 nElements   = dataspace.getSimpleExtentNpoints();
   ossim_int32 nMembers    = compound.getNmembers();
   ossim_uint64 size       = compound.getSize();
   ossim_int32 memberIdx   = 0;
   ossim_int32 elementIdx   = 0;
   H5::DataType compType = dataset.getDataType();
   std::vector<char> compData(size*nElements);
   dataset.read((void*)&compData.front(),compType);
   char* compDataPtr = &compData.front();

   if(dimensions!=1)
   {
      return;
   }

   for(;elementIdx<nElements;++elementIdx)
   {
      std::string elementPrefix = prefix;//+"."+"element"+ossimString::toString(elementIdx);
      //std::cout << "ELEMENTS: " << elements << std::endl;
      for(memberIdx=0;memberIdx < nMembers;++memberIdx)
      {
         H5::DataType dataType = compound.getMemberDataType(memberIdx);
         H5std_string memberName = compound.getMemberName(memberIdx);
         ossim_uint32 memberOffset = compound.getMemberOffset(memberIdx) ;
         std::string newPrefix = elementPrefix + memberName;

         switch(dataType.getClass())
         {
            case H5T_COMPOUND:
            {
              H5::CompType compoundType(dataset);
             // dumpCompoundTypeInfo(compoundType, newPrefix);
              dumpCompound(dataset, compoundType, newPrefix);
              break;            
            }
            case H5T_INTEGER:
            {
               H5::IntType dataType = compound.getMemberIntType(memberIdx);
               dumpIntType(dataType, &compDataPtr[memberOffset], newPrefix);
               //ossim_hdf5::printIntType(dataset, dataType, &compDataPtr[memberOffset], newPrefix, out);
               break;            
            }
            case H5T_FLOAT:
            {
               H5::FloatType dataType = compound.getMemberFloatType(memberIdx);
               dumpFloatType(dataType, &compDataPtr[memberOffset], newPrefix);
               break;            
            }
            case H5T_TIME:
            case H5T_STRING:
            {
               H5::StrType dataType = compound.getMemberStrType(memberIdx);
               dumpStringType(dataType, &compDataPtr[memberOffset], newPrefix);
//               ossim_hdf5::printStrType(dataset, dataType, &compDataPtr[memberOffset], newPrefix, out);
               break;            
            }
            case H5T_BITFIELD:
            {
//               out << newPrefix << ": <H5T_BITFIELD NOT HANDLED>\n"; 
               break;            
            }
            case H5T_OPAQUE:
            {
//               out << newPrefix << ": <H5T_OPAQUE NOT HANDLED>\n"; 
               break;            
            }
            case H5T_REFERENCE:
            {
//               out << newPrefix << ": <H5T_REFERENCE NOT HANDLED>\n"; 
               break;            
            }
            case H5T_ENUM:
            {
               H5::EnumType dataType = compound.getMemberEnumType(memberIdx);
               dumpEnumTypeInfo(dataType, newPrefix);
//               ossim_hdf5::printEnumType(dataset, dataType, newPrefix, out);
               break;            
            }
            case H5T_VLEN:
            {
//               out << newPrefix << ": <H5T_VLEN NOT HANDLED>\n"; 
               break;            
            }
            case H5T_ARRAY:
            {
                H5::ArrayType dataType = compound.getMemberArrayType(memberIdx);
                dumpArrayType(dataType, &compDataPtr[memberOffset], newPrefix);
               break;            
            }
            case H5T_NO_CLASS:
            default:
            {
               break;            
            }
         }
      }
      compDataPtr+=size;
   }

}

void ossimHdf5Info::dumpCompoundTypeInfo(const H5::CompType& compound,
                                         const std::string& prefix) const
{
   #if 0
   try
   {
     // H5::CompType compound(dataset);
      ossim_int32 nMembers    = compound.getNmembers();
      ossim_int32 memberIdx   = 0;
      ostringstream typePrefix;
      typePrefix << prefix << "compound_type.";
      m_kwl.addPair(prefix, string("type"), string("compound"));
   
      for(memberIdx=0;memberIdx < nMembers;++memberIdx)
      {
         H5::DataType dataType (compound.getMemberDataType(memberIdx));
         H5std_string memberName (compound.getMemberName(memberIdx));
         ostringstream newPrefix;
         newPrefix<<typePrefix.str() <<memberName<< ".";

         H5T_class_t class_type = dataType.getClass();
         m_kwl.addPair(newPrefix.str(), string("class_type"), m_hdf5->getDatatypeClassType(class_type));

         switch(class_type)
         {
            case H5T_INTEGER:
            {
               H5::IntType dataType = compound.getMemberIntType(memberIdx);
               //dumpIntType(dataset, dataType, &compDataPtr[memberOffset], newPrefix, out);
               
               break;
            }
            case H5T_FLOAT:
            {
               H5::FloatType dataType = compound.getMemberFloatType(memberIdx);
               break;
            }
            case H5T_ENUM:
            {
               H5::EnumType enudataType = compound.getMemberEnumType(memberIdx);
               dumpEnumTypeInfo(enudataType, newPrefix.str());
               break;
            }
            case H5T_ARRAY:
            {
               H5::ArrayType arrdataType = compound.getMemberArrayType(memberIdx);
               dumpArrayTypeInfo(arrdataType, newPrefix.str());
               break;
            }
            default:
            {
               break;
            }
         }
      }
   }
   catch( const H5::Exception& e )
   {
      ossimNotify(ossimNotifyLevel_WARN)<<e.getDetailMsg();
   }
   #endif
}

void ossimHdf5Info::dumpEnumTypeInfo(H5::EnumType enumType,
                                     const std::string& prefix) const
{
   try
   {
      ossim_int32 nEnumMembers = enumType.getNmembers();
      ossim_int32 enumTypeSize = enumType.getSize();
      if (!nEnumMembers || !enumTypeSize)
         return;

      char* enum_value = new char [enumTypeSize];
      ostringstream kwl_value;

      for(ossim_int32 i=0;i<nEnumMembers;++i)
      {
         enumType.getMemberValue(i, &enum_value);
         H5std_string name = enumType.nameOf(&enum_value, enumTypeSize);
         if (i==0)
            kwl_value << name;
         else
            kwl_value << ", " << name;
      }
      m_kwl.addPair(prefix, string("enumerations"), kwl_value.str());
      delete [] enum_value;
   }
   catch( const H5::Exception& e )
   {
      ossimNotify(ossimNotifyLevel_WARN)<<e.getDetailMsg();
   }
}


void ossimHdf5Info::dumpArrayTypeInfo(H5::ArrayType arrayType,
                                      const std::string& prefix) const
{
   try
   {
      ossim_uint32 arrayNdims = arrayType.getArrayNDims();
      string ndimsstr = ossimString::toString(arrayNdims);
      m_kwl.addPair(prefix, "rank", ndimsstr);

      if (arrayNdims)
      {
         std::vector<hsize_t> dims(arrayNdims);
         arrayType.getArrayDims(&dims.front());
         ostringstream kwl_value;
         for(ossim_uint32 i=0;i<arrayNdims;++i)
         {
            if (i==0)
               kwl_value << dims[i];
            else
               kwl_value << ", " << dims[i];
         }
         m_kwl.addPair(prefix, string("dimensions"), kwl_value.str());
      }
   }
   catch( const H5::Exception& e )
   {
      ossimNotify(ossimNotifyLevel_WARN)<<e.getDetailMsg();
   }
}

void ossimHdf5Info::dumpNumericalTypeInfo(const H5::DataSet& dataset,
                                          ossimByteOrder byteOrder,
                                          const std::string& prefix) const
{
   try
   {
      ossimScalarType stype = m_hdf5->getScalarType(dataset);
      ossimString sct = ossimScalarTypeLut::instance()->getEntryString(stype);
      m_kwl.addPair(prefix, string(ossimKeywordNames::SCALAR_TYPE_KW), sct.string());

      std::string byteOrderString = "little_endian";
      if ( byteOrder == OSSIM_BIG_ENDIAN )
         byteOrderString = "big_endian";
      m_kwl.addPair(prefix, string(ossimKeywordNames::BYTE_ORDER_KW), byteOrderString);
   }
   catch( const H5::Exception& e )
   {
      ossimNotify(ossimNotifyLevel_WARN)<<e.getDetailMsg();
   }
}

void ossimHdf5Info::dumpIntType(const H5::IntType& dataType,
                                const char* dataPtr,
                                const std::string& prefix)const
{
   std::string strValue;
   if(ossimHdf5::intTypeToString(strValue, dataType, dataPtr))
   {
      m_kwl.addPair(prefix, strValue);
   }

}

void ossimHdf5Info::dumpFloatType(const H5::FloatType& dataType,
                                  const char* dataPtr,
                                  const std::string& prefix)const
{
   std::string strValue;

   if(ossimHdf5::floatTypeToString(strValue, dataType, dataPtr))
   {
      m_kwl.addPair(prefix, strValue);
   }
}

void ossimHdf5Info::dumpStringType(const H5::StrType& dataType,
                                   const char* dataPtr,
                                   const std::string& prefix)const
{
   std::string strValue;
   if(ossimHdf5::stringTypeToString(strValue, dataType, dataPtr))
   {
      m_kwl.addPair(prefix, strValue);
   }
}

void ossimHdf5Info::dumpArrayType( H5::ArrayType& dataType,
                                   const char* dataPtr,
                                   const std::string& prefix)const
{
   ossim_uint32 arrayNdims = dataType.getArrayNDims();
//   ossimByteOrder order = m_hdf5->getByteOrder(dataType);
//   ossimEndian endian;
//   bool swapOrder = (order!=ossim::byteOrder());
   H5::DataType superType = dataType.getSuper();
   //out << prefix<<".class_type: " << ossim_hdf5::getDatatypeClassType( dataType.getClass() ) << "\n";
   if(arrayNdims)
   {
      std::vector<hsize_t> dims(arrayNdims);
      dataType.getArrayDims(&dims.front());
      if(dims.size() > 0)
      {
         std::stringstream dimOut;
         ossimString dimString;
         ossim_uint32 idx = 1;
         ossim_uint32 nArrayElements = dims[0];
         std::copy(dims.begin(), --dims.end(),
            std::ostream_iterator<hsize_t>(dimOut,","));
         for(;idx<dims.size();++idx)
         {
          nArrayElements*=dims[idx]; 
         }

         dimString = ossimString("(") + dimOut.str() + ossimString::toString(dims[dims.size()-1])+")";  
         m_kwl.addPair(prefix+".dimensions", dimString);

         ossim_uint32 typeSize = superType.getSize();
         switch(superType.getClass())
         {
            case H5T_STRING:
            {
               std::ostringstream out;
               ossimString newPrefix = prefix+".values";
               out<<"(";
               const char* startPtr = 0;
               const char* endPtr = 0;
               const char* mem = 0;
               H5T_str_t       pad;
               ossim_int32 strSize = 0;
               for(idx=0;idx<nArrayElements;++idx)
               {
                  mem = ((const char*)dataPtr) + (idx * typeSize);
                  if(superType.isVariableStr())
                  {
                     startPtr = *(char**) mem;
                     if(startPtr) strSize = std::strlen(startPtr);
                     else strSize = 0;
                  }
                  else
                  {
                     startPtr = mem;
                  }
                  if(startPtr)
                  {
                    ossim_uint32 charIdx = 0;
                    endPtr = startPtr;
                    for (; ((charIdx < strSize) && (*endPtr)); ++charIdx,++endPtr);

                  }
                  ossimString value = ossimString(startPtr, endPtr);
                  if(idx == 0)
                  {
                     out << "\""<<value<< "\"";
                  }
                  else 
                  {
                     out << ", \""<<value<< "\"";
                  }
               }
               out << ")";
               //out<<prefix<<".array_type: H5T_STRING\n";
//               out<<prefix<<".values: <NOT SUPPORTED>";
               m_kwl.addPair(newPrefix, out.str());
               m_kwl.addPair(prefix+".array_type", "H5T_STRING");

               break;
            }
            case H5T_INTEGER:
            {
               ostringstream out;
               H5::IntType* dataTypePtr = (H5::IntType*)(&superType);
               ossimByteOrder order = m_hdf5->getByteOrder(*dataTypePtr);
               ossimEndian endian;
               bool swapOrder = (order!=ossim::byteOrder());
               ossimString newPrefix = prefix+".values";
               out<<"(";
               for(idx=0;idx<nArrayElements;++idx)
               {
                  std::string value;
                  ossimHdf5::intTypeToString(value, *dataTypePtr, dataPtr);
                  if((idx + 1) <nArrayElements)
                  {
                     out << value << ", ";
                  }
                  else
                  {
                     out << value;
                  }
                  dataPtr+=typeSize;
               }
               out << ")";
               m_kwl.addPair(newPrefix, out.str());
               m_kwl.addPair(prefix+".array_type", "H5T_INTEGER");
              break;
            }
            case H5T_FLOAT:
            {
               ostringstream out;
               H5::FloatType* dataTypePtr = (H5::FloatType*)(&superType);
               ossimByteOrder order = m_hdf5->getByteOrder(*dataTypePtr);
               ossimEndian endian;
               bool swapOrder = (order!=ossim::byteOrder());

               //out<<prefix<<".array_type: H5T_INTEGER\n";
               ossimString newPrefix = prefix+".values";
               out<<"(";
               for(idx=0;idx<nArrayElements;++idx)
               {
                  std::string value;
                  ossimHdf5::floatTypeToString(value, *dataTypePtr, dataPtr);
                  if((idx + 1) <nArrayElements)
                  {
                     out << value << ", ";
                  }
                  else
                  {
                     out << value;
                  }
                  dataPtr+=typeSize;
               }
               out << ")";
               m_kwl.addPair(newPrefix, out.str());
               m_kwl.addPair(prefix+".array_type", "H5T_FLOAT");
               break;
            }
            default:
            {
               break;
            }
         }

      }
   }
  
}


void ossimHdf5Info::dumpNumerical(const H5::DataSet& dataset,
                                  const char* dataPtr,
                                  const std::string& prefix) const
{
   try
   {
      H5::IntType dataType = dataset.getIntType();
      ossimByteOrder order = m_hdf5->getByteOrder(&dataset);
      ossimEndian endian;
      bool swapOrder = (order!=ossim::byteOrder());

      ossimString valueStr;
      ossimScalarType scalarType = m_hdf5->getScalarType(dataset);
      switch(scalarType)
      {
      case OSSIM_UINT8:
      {
         ossim_uint8 value = *reinterpret_cast<const ossim_uint8*>(dataPtr);
         valueStr = ossimString::toString(value).string();
         break;
      }
      case OSSIM_SINT8:
      {
         ossim_int8 value = *reinterpret_cast<const ossim_int8*>(dataPtr);
         valueStr = ossimString::toString(value);
         break;
      }
      case OSSIM_UINT16:
      {
         ossim_uint16 value = *reinterpret_cast<const ossim_uint16*>(dataPtr);
         if(swapOrder)
            endian.swap(value);
         valueStr = ossimString::toString(value);
         break;
      }
      case OSSIM_SINT16:
      {
         ossim_int16 value = *reinterpret_cast<const ossim_int16*>(dataPtr);
         if(swapOrder)
            endian.swap(value);
         valueStr = ossimString::toString(value);
         break;
      }
      case OSSIM_UINT32:
      {
         ossim_uint32 value = *reinterpret_cast<const ossim_uint32*>(dataPtr);
         if(swapOrder)
            endian.swap(value);
         valueStr = ossimString::toString(value);
         break;
      }
      case OSSIM_SINT32:
      {
         ossim_int32 value = *reinterpret_cast<const ossim_int32*>(dataPtr);
         if(swapOrder)
            endian.swap(value);
         valueStr = ossimString::toString(value);
         break;
      }
      case OSSIM_UINT64:
      {
         ossim_uint64 value = *reinterpret_cast<const ossim_uint64*>(dataPtr);
         if(swapOrder)
            endian.swap(value);
         valueStr = ossimString::toString(value);
         break;
      }
      case OSSIM_FLOAT32:
      {
         ossim_float32 value = *reinterpret_cast<const ossim_int64*>(dataPtr);
         if(swapOrder)
            endian.swap(value);
         valueStr = ossimString::toString(value);
         break;
      }
      case OSSIM_FLOAT64:
      {
         ossim_float64 value = *reinterpret_cast<const ossim_int64*>(dataPtr);
         if(swapOrder)
            endian.swap(value);
         valueStr = ossimString::toString(value);
         break;
      }
      default:
      {
         valueStr = "<UNHANDLED SCALAR TYPE>";
         break;
      }
      }

      m_kwl.addPair(prefix, string("value"), valueStr.string());
   }
   catch( const H5::Exception& e )
   {
      ossimNotify(ossimNotifyLevel_WARN)<<e.getDetailMsg();
   }
}



