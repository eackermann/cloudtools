#include "DataMgr.h"

#include <pcl/point_cloud.h>
#include <pcl/point_types.h>
#include <pcl/io/pcd_io.h>
#include <pcl/PCLPointCloud2.h>


DataMgr::DataMgr(RichParameterSet* _para)
{
	para = _para;
}


DataMgr::~DataMgr(void)
{
}

void DataMgr::clearCMesh(CMesh& mesh)
{
	mesh.face.clear();
	mesh.fn = 0;
	mesh.vert.clear();
	mesh.vn = 0;
	mesh.bbox = Box3f();
}

bool DataMgr::isSamplesEmpty()
{
	return samples.vert.empty();
}


bool DataMgr::isOriginalEmpty()
{
	return original.vert.empty();
}

bool DataMgr::isSkeletonEmpty()
{
  return skeleton.isEmpty();
}

inline float get_float(uint8_t* pt, const pcl::PCLPointField& f)
{
  return *(float*)&pt[f.offset];
}

inline double get_double(uint8_t* pt, const pcl::PCLPointField& f)
{
  return *(double*)&pt[f.offset];
}

inline float get_field(uint8_t* pt, const pcl::PCLPointField& f)
{
  if (f.datatype == pcl::PCLPointField::PointFieldTypes::FLOAT32) {
    return get_float(pt, f);
  } else if (f.datatype == pcl::PCLPointField::PointFieldTypes::FLOAT64) {
    return static_cast<float>(get_double(pt, f));
  }
}

void DataMgr::loadPCD(const std::string& filename, bool is_sample)
{
  pcl::PCLPointCloud2 cloud;
  pcl::PCDReader in;
  in.read(filename, cloud);

  bool has_point = false;
  bool has_normal = false;  
  uint32_t findex[6]; // x y z nx ny nz
  size_t N = cloud.width * cloud.height;
  int i = 0;
  for (auto f : cloud.fields) {
    if (f.name == "x") {
      has_point = true;
      findex[0] = i;
    } else if (f.name == "y") {
      has_point = true;
      findex[1] = i;
    } else if (f.name == "z") {
      has_point = true;
      findex[2] = i;
    } else if (f.name == "normal_x") {
      has_normal = true;
      findex[3] = i;
    } else if (f.name == "normal_y") {
      has_normal = true;
      findex[4] = i;
    } else if (f.name == "normal_z") {
      has_normal = true;
      findex[5] = i;
    }
    ++i;
  }
  if (!has_point) {
    std::cerr << "PCD must contains x,y,z fields. NO POINTS LOADED." << std::endl;
    return;
  }

  if (!is_sample) {
    clearCMesh(original);
  } else {
    clearCMesh(samples);
  }
  curr_file_name = filename.c_str();
  // iterate through the cloud and extract the points and normals
  auto vi = vcg::tri::Allocator<CMesh>::AddVertices(is_sample ? samples : original,N);
  uint8_t* data = &cloud.data[0];
  for (size_t i = 0, o = 0; i < N; ++i, o += cloud.point_step) {
    auto pt = &data[o];
    vi->P() = CMesh::CoordType( get_field(pt, cloud.fields[findex[0]]),
                                get_field(pt, cloud.fields[findex[1]]),
                                get_field(pt, cloud.fields[findex[2]]));
    if (has_normal) {
      vi->N() = CMesh::CoordType( get_field(pt, cloud.fields[findex[3]]),
                                  get_field(pt, cloud.fields[findex[4]]),
                                  get_field(pt, cloud.fields[findex[5]]));
    }
    vi++;
  }
}

void DataMgr::savePCD(const std::string& filename, CMesh& mesh)
{
  
}

void DataMgr::loadPlyToOriginal(QString fileName)
{
	clearCMesh(original);
	curr_file_name = fileName;

	int mask= tri::io::Mask::IOM_VERTCOORD + tri::io::Mask::IOM_VERTNORMAL ;

	int err = tri::io::Importer<CMesh>::Open(original, curr_file_name.toAscii().data(), mask);  
	if(err) 
	{
		cout << "Failed reading mesh: " << err << "\n";
		return;
	}  
	cout << "points loaded\n";


	CMesh::VertexIterator vi;
	int idx = 0;
	for(vi = original.vert.begin(); vi != original.vert.end(); ++vi)
	{
		vi->bIsOriginal = true;
		vi->m_index = idx++;
		//vi->N() = Point3f(0, 0, 0);
		original.bbox.Add(vi->P());
	}
	original.vn = original.vert.size();
}

void DataMgr::loadPlyToSample(QString fileName)
{
	clearCMesh(samples);
	curr_file_name = fileName;

	int mask= tri::io::Mask::IOM_VERTCOORD + tri::io::Mask::IOM_VERTNORMAL ;
	mask += tri::io::Mask::IOM_VERTCOLOR;
	mask += tri::io::Mask::IOM_BITPOLYGONAL;

	int err = tri::io::Importer<CMesh>::Open(samples, curr_file_name.toAscii().data(), mask);  
	if(err) 
	{
		cout << "Failed reading mesh: " << err << "\n";
		return;
	}  

	CMesh::VertexIterator vi;
	int idx = 0;
	for(vi = samples.vert.begin(); vi != samples.vert.end(); ++vi)
	{
		vi->bIsOriginal = false;
		vi->m_index = idx++;
		samples.bbox.Add(vi->P());
	}
	samples.vn = samples.vert.size();
}

void DataMgr::loadXYZN(QString fileName)
{
  clearCMesh(samples);
  ifstream infile;
  infile.open(fileName.toStdString().c_str());

  int i = 0;
  while(!infile.eof())
  {
    CVertex v;
    float temp = 0.;
    for (int j=0; j<3; j++)
    {

      infile >> temp;
      v.P()[j] = temp;
    }


    for (int j=0; j<3; j++) {
      infile >> v.N()[j];
    }

    v.m_index = i++;

    samples.vert.push_back(v);
    samples.bbox.Add(v.P());
  }

 // mesh.vert.erase(mesh.vert.end()-1);
  samples.vert.pop_back();
  samples.vn = samples.vert.size();

  infile.close();



}

void DataMgr::loadImage(QString fileName)
{

	//image = cv::imread(fileName.toAscii().data());

	////cv::namedWindow("image", CV_WINDOW_AUTOSIZE);
	////cv::imshow("image", image);

	//clearCMesh(samples);
	//clearCMesh(original);
	//int cnt = 0;
	//for (int i = 0; i < image.rows; i++)
	//{
	//	for (int j = 0; j < image.cols; j++)
	//	{
	//		cv::Vec3b intensity = image.at<cv::Vec3b>(i, j);
	//		Point3f p;
	//		Color4b c;
	//		c.Z() = 1;
	//		p.X() = c.X() = intensity.val[0];
	//		p.Y() = c.Y() = intensity.val[1];
	//		p.Z() = c.Z() = intensity.val[2];
	//		CVertex new_v;
	//		new_v.P() = p;
	//		new_v.C() = c;
	//		new_v.m_index = cnt++;

	//		samples.vert.push_back(new_v);
	//		samples.bbox.Add(p);

	//		new_v.bIsOriginal = true;
	//		original.vert.push_back(new_v);
	//		original.bbox.Add(p);
	//	}
	//}
	//samples.vn = samples.vert.size();
	//original.vn = samples.vn;

	//cv::waitKey();

}

CMesh* DataMgr::getCurrentSamples()
{
  if(&samples == NULL)
  {
    //cout << "DataMgr::getCurrentSamples samples = NULL!!" <<endl;
    return NULL;
  }

	if(samples.vert.empty())
	{
		//cout << "DataMgr::getCurrentSamples samples.vert.empty()!!" <<endl;
		//return NULL;
    return & samples;
	}

	return & samples;
}

CMesh* DataMgr::getCurrentOriginal()
{
  if(&original == NULL)
  {
    //cout << "DataMgr::getCurrentOriginal() samples = NULL!!" <<endl;
    return NULL;
  }

	if(original.vert.empty())
	{
		//cout << "DataMgr::getCurrentOriginal() original.vert.empty()!!" <<endl;
		return & original;
	}

	return & original;
}

Skeleton* DataMgr::getCurrentSkeleton()
{
	return & skeleton;
}


void DataMgr::recomputeBox()
{
	samples.bbox.SetNull();
	original.bbox.SetNull();

	CMesh::VertexIterator vi;
	for(vi = samples.vert.begin(); vi != samples.vert.end(); ++vi) 
	{
		if (vi->is_skel_ignore)
		{
			continue;
		}
		samples.bbox.Add(vi->P());
	}

	for(vi = original.vert.begin(); vi != original.vert.end(); ++vi) 
	{
		original.bbox.Add(vi->P());
	}
}

double DataMgr::getInitRadiuse()
{
	double init_para = para->getDouble("Init Radius Para");
	if (!isOriginalEmpty())
	{
		Box3f box = original.bbox;
		if ( abs(box.min.X() - box.max.X()) < 1e-5 ||   
			abs(box.min.Y() - box.max.Y()) < 1e-5 ||   
			abs(box.min.Z() - box.max.Z()) < 1e-5 )
		{
			double diagonal_length = sqrt((box.min - box.max).SquaredNorm());
			double original_size = sqrt(double(original.vn));
			init_radius = 2 * init_para * diagonal_length / original_size;
		}
		else
		{
			double diagonal_length = sqrt((box.min - box.max).SquaredNorm());
			double original_size = pow(double(original.vn), 0.333);
			init_radius = init_para * diagonal_length / original_size;
		}
	}

  auto r = DoubleValue(init_radius);
  global_paraMgr.setGlobalParameter("CGrid Radius", r);
  global_paraMgr.setGlobalParameter("Initial Radius", r);

	return init_radius;
}


void DataMgr::downSamplesByNum(bool use_random_downsample)
{
	if (isOriginalEmpty() && !isSamplesEmpty())
	{
		subSamples();
		return;
	}

	if (isOriginalEmpty())
	{
		return;
	}

	int want_sample_num = para->getDouble("Down Sample Num");

	if (want_sample_num > original.vn)
	{
		want_sample_num = original.vn;
	}

	clearCMesh(samples);
	samples.vn = want_sample_num;

	vector<int> nCard = GlobalFun::GetRandomCards(original.vert.size());
	for(int i = 0; i < samples.vn; i++) 
	{
		int index = nCard[i]; //could be not random!

    if (!use_random_downsample)
    {
      index = i;
    }

		CVertex& v = original.vert[index];
		samples.vert.push_back(v);
		samples.bbox.Add(v.P());
	}

	CMesh::VertexIterator vi;
	for(vi = samples.vert.begin(); vi != samples.vert.end(); ++vi)
	{
		vi->bIsOriginal = false;
	}

  getInitRadiuse();
}

void DataMgr::subSamples()
{
	clearCMesh(original);

	CMesh::VertexIterator vi;
	original.vn = samples.vert.size();
	original.bbox.SetNull();
	for(vi = samples.vert.begin(); vi != samples.vert.end(); ++vi)
	{
		CVertex v = (*vi);
		v.bIsOriginal = true;
		original.vert.push_back(v);
		original.bbox.Add(v.P());
	}

	downSamplesByNum();
  getInitRadiuse();
}


void DataMgr::savePly(QString fileName, CMesh& mesh)
{
	int mask= tri::io::Mask::IOM_VERTCOORD + tri::io::Mask::IOM_VERTNORMAL ;
	mask += tri::io::Mask::IOM_VERTCOLOR;
	mask += tri::io::Mask::IOM_BITPOLYGONAL;

	if (fileName.endsWith("ply"))
		tri::io::ExporterPLY<CMesh>::Save(mesh, fileName.toAscii().data(), mask, false);
}

void DataMgr::normalizeROSA_Mesh(CMesh& mesh)
{
	if (mesh.vert.empty())
	{
		return;
	}
	Box3f box = mesh.bbox;
	mesh.bbox.SetNull();
	float max_x = abs((box.min - box.max).X());
	float max_y = abs((box.min - box.max).Y());
	float max_z = abs((box.min - box.max).Z());
	float max_length = max_x > max_y ? max_x : max_y;
	max_length = max_length > max_z ? max_length : max_z;

	for(int i = 0; i < mesh.vert.size(); i++)
	{
		Point3f& p = mesh.vert[i].P();

		p -= box.min;
		p /= max_length;

		p = (p - Point3f(0.5, .5, .5));

		mesh.vert[i].N().Normalize(); 
		mesh.bbox.Add(p);
	}
}

Box3f DataMgr::normalizeAllMesh()
{
	Box3f box;
	if (!isSamplesEmpty())
	{
		for (int i = 0; i < samples.vert.size(); i++)
		{
			box.Add(samples.vert[i].P());
		}
	}
	if (!isOriginalEmpty())
	{
		for (int i = 0; i < original.vert.size(); i++)
		{
			box.Add(original.vert[i].P());
		}
		original.bbox =box;
	}
	samples.bbox = box;

	normalizeROSA_Mesh(samples);
	normalizeROSA_Mesh(original);

	recomputeBox();
	getInitRadiuse();

	return samples.bbox;
}


void DataMgr::eraseRemovedSamples()
{
	int cnt = 0;
	vector<CVertex> temp_mesh;
	for (int i = 0; i < samples.vert.size(); i++)
	{
		CVertex& v = samples.vert[i];
		if (!v.is_skel_ignore)
		{
			temp_mesh.push_back(v);
		}
	}

	samples.vert.clear();
	samples.vn = temp_mesh.size();
	for (int i = 0; i < temp_mesh.size(); i++)
	{
		temp_mesh[i].m_index = i;
		samples.vert.push_back(temp_mesh[i]);
	}

}

void DataMgr::clearData()
{
	clearCMesh(original);
	clearCMesh(samples);
	skeleton.clear();
}

void DataMgr::recomputeQuad()
{
	for (int i = 0; i < samples.vert.size(); i++)
	{
		samples.vert[i].recompute_m_render();
	}
}


void DataMgr::saveSkeletonAsSkel(QString fileName)
{
	ofstream outfile;
	outfile.open(fileName.toStdString().c_str());

	ostringstream strStream; 

	strStream << "ON " << original.vert.size() << endl;
	for(int i = 0; i < original.vert.size(); i++)
	{
		CVertex& v = original.vert[i];
		strStream << v.P()[0] << "	" << v.P()[1] << "	" << v.P()[2] << "	";
		strStream << v.N()[0] << "	" << v.N()[1] << "	" << v.N()[2] << "	" << endl;
	}
	strStream << endl;

	strStream << "SN " << samples.vert.size() << endl;
	for(int i = 0; i < samples.vert.size(); i++)
	{
		CVertex& v = samples.vert[i];
		strStream << v.P()[0] << "	" << v.P()[1] << "	" << v.P()[2] << "	";
		strStream << v.N()[0] << "	" << v.N()[1] << "	" << v.N()[2] << "	" << endl;
	}
	strStream << endl;

	strStream << "CN " << skeleton.branches.size() << endl;
	for (int i = 0; i < skeleton.branches.size(); i++)
	{
		Branch& branch = skeleton.branches[i];
		strStream << "CNN " << branch.curve.size() << endl;
		for (int j = 0; j < branch.curve.size(); j++)
		{
			strStream << branch.curve[j][0] << "	" << branch.curve[j][1] << "	" << branch.curve[j][2] << "	" << endl;
		}
	}
	strStream << endl;

	strStream << "EN " << 0 << endl;
	strStream << endl;


	strStream << "BN " << 0 << endl;
	strStream << endl;


	strStream << "S_onedge " << samples.vert.size() << endl;
	for(int i = 0; i < samples.vert.size(); i++)
	{
		CVertex& v = samples.vert[i];
		strStream << v.is_fixed_sample << "	"; 
	}
	strStream << endl;

	strStream << "GroupID " << samples.vert.size() << endl;
	for(int i = 0; i < samples.vert.size(); i++)
	{
		int id = samples.vert[i].m_index;//group_id no use now
		strStream << id << "	"; 
	}
	strStream << endl;

	//strStream << "SkelRadius " << 0 << endl;
	//strStream << endl;
  
  strStream << "SkelRadius " << skeleton.size << endl;
  for (int i = 0; i < skeleton.branches.size(); i++)
  {
    for (int j = 0; j < skeleton.branches[i].curve.size(); j++)
    {
      double skel_radius = skeleton.branches[i].curve[j].skel_radius;
      strStream << skel_radius << "	"; 
    }
  }
  strStream << endl;

	strStream << "Confidence_Sigma	" << samples.vert.size() << endl;
	for(int i = 0; i < samples.vert.size(); i++)
	{
		double sigma = samples.vert[i].eigen_confidence;
		strStream << sigma << "	"; 
	}
	strStream << endl;

	strStream << "SkelRadius2 " << 0 << endl;
	strStream << endl;

	strStream << "Alpha " << 0 << endl;
	strStream << endl;

	strStream << "Sample_isVirtual " << samples.vert.size() << endl;
	for(int i = 0; i < samples.vert.size(); i++)
	{
		CVertex& v = samples.vert[i];
		strStream << v.is_skel_virtual << "	"; 
	}
	strStream << endl;

	strStream << "Sample_isBranch " << samples.vert.size() << endl;
	for(int i = 0; i < samples.vert.size(); i++)
	{
		CVertex& v = samples.vert[i];
		strStream << v.is_skel_branch << "	"; 
	}
	strStream << endl;

  strStream << "Sample_radius " << samples.vert.size() << endl;
  for(int i = 0; i < samples.vert.size(); i++)
  {
    CVertex& v = samples.vert[i];
    strStream << 0 << "	"; 
  }
  strStream << endl;

	strStream << "Skel_isVirtual " << skeleton.size << endl;
	for (int i = 0; i < skeleton.branches.size(); i++)
	{
		for (int j = 0; j < skeleton.branches[i].curve.size(); j++)
		{
			bool is_virtual = skeleton.branches[i].curve[j].is_skel_virtual;
			strStream << is_virtual << "	"; 
		}
	}
	strStream << endl;


  strStream << "Corresponding_sample_index " << skeleton.size << endl;
  for (int i = 0; i < skeleton.branches.size(); i++)
  {
    for (int j = 0; j < skeleton.branches[i].curve.size(); j++)
    {
      int index = skeleton.branches[i].curve[j].m_index;
      strStream << index << "	"; 
    }
  }
  strStream << endl;

	outfile.write( strStream.str().c_str(), strStream.str().size() ); 
	outfile.close();
}




void DataMgr::loadSkeletonFromSkel(QString fileName)
{
	clearCMesh(samples);
	clearCMesh(original);
	skeleton.clear();

	ifstream infile;
	infile.open(fileName.toStdString().c_str());

	stringstream sem; 
	sem << infile.rdbuf(); 

	string str;
	int num;
	int num2;

	sem >> str;
	if (str == "ON")
	{
		sem >> num;
		bool is_same_original = false;
		if (num == original.vn)
		{
			is_same_original = true;
		}
		if (is_same_original)
		{
			double temp;
			for (int i = 0; i < num * 6; i++)
			{
				sem >> temp;
			}
		}
		else
		{
			for (int i = 0; i < num; i++)
			{
				CVertex v;
				v.bIsOriginal = true;
				v.m_index = i;
				sem >> v.P()[0] >> v.P()[1] >> v.P()[2];
				sem >> v.N()[0] >> v.N()[1] >> v.N()[2];
				original.vert.push_back(v);
				original.bbox.Add(v.P());
			}
			original.vn = original.vert.size();
		}
	}

	sem >> str;
	if (str == "SN")
	{
		sem >> num;
		for (int i = 0; i < num; i++)
		{
			CVertex v;
			v.bIsOriginal = false;
			v.m_index = i;
			sem >> v.P()[0] >> v.P()[1] >> v.P()[2];
			sem >> v.N()[0] >> v.N()[1] >> v.N()[2];
			samples.vert.push_back(v);
			samples.bbox.Add(v.P());
		}
		samples.vn = samples.vert.size();
	}


	sem >> str;
	if (str == "CN")
	{
		sem >> num;
		for (int i = 0; i < num; i++)
		{
			Branch branch;
			sem >> str;
			sem >> num2;
			for(int j = 0; j < num2; j++)
			{
				Point3f p;
				CVertex v;
				sem >> p[0] >> p[1] >> p[2];
				v.P() = p;
				branch.curve.push_back(v);
			}
			skeleton.branches.push_back(branch);
		}
	}

	sem >> str;
	if (str == "EN")
	{
    sem >> num;
    for (int i = 0; i < num; i++)
    {
      int a, b;
      sem >> a >> b;
    }
	}

	sem >> str;
	if (str == "BN")
	{
    sem >> num;
    for (int i = 0; i < num; i++)
    {
      sem >> str;
      sem >> num2;

      for(int j = 0; j < num2; j++)
      {
        int id;
        sem >> id;

    }
	}

	if (!sem.eof())
	{
		sem >> str;
		if (str == "S_onedge")
		{
			sem >> num;
			for (int i = 0; i < num; i++)
			{
				bool b;
				sem >> b;
				samples.vert[i].is_fixed_sample = b;
			}
		}
	}

	sem >> str;
	if (str == "GroupID")
	{
		sem >> num;
		for (int i = 0; i < num; i++)
		{
			int id;
			sem >> id;

      }
		}
	}

	sem >> str;
	if (str == "SkelRadius")
	{
		sem >> num;

    if (num > 1)
    {
      double radius;
      for (int i = 0; i < skeleton.branches.size(); i++)
      {
        for (int j = 0; j < skeleton.branches[i].curve.size(); j++)
        {
          sem >> radius;
          skeleton.branches[i].curve[j].skel_radius = radius;
        }
      }
    }

	}

	sem >> str;
	if (str == "Confidence_Sigma")
	{
		sem >> num;
		for (int i = 0; i < num; i++)
		{
			double sigma;
			sem >> sigma;
			samples.vert[i].eigen_confidence = sigma;
		}
	}

	sem >> str;
	if (str == "SkelRadius2")
	{
		sem >> num;

    if (num > 1)
    {
      double radius;
      for (int i = 0; i < skeleton.branches.size(); i++)
      {
        for (int j = 0; j < skeleton.branches[i].curve.size(); j++)
        {
          sem >> radius;
          //skeleton.branches[i].curve[j].skel_radius = radius;
        }
      }
    }

	}

	sem >> str;
	if (str == "Alpha")
	{
		sem >> num;
    double Alpha;
    if (num > 1)
    {
      for (int i = 0; i < skeleton.branches.size(); i++)
      {
        for (int j = 0; j < skeleton.branches[i].curve.size(); j++)
        {
          sem >> Alpha;
          //skeleton.curves[i][j].alpha = Alpha;
        }
      }
    }

	}

	if (!sem.eof())
	{
		sem >> str;
		if (str == "Sample_isVirtual")
		{
			sem >> num;
			for (int i = 0; i < num; i++)
			{
				bool b;
				sem >> b;
				samples.vert[i].is_skel_virtual = b;
			}
		}
	}

	if (!sem.eof())
	{
		sem >> str;
		if (str == "Sample_isBranch")
		{
			sem >> num;
			for (int i = 0; i < num; i++)
			{
				bool b;
				sem >> b;
				samples.vert[i].is_skel_branch = b;
			}
		}
	}

	sem >> str;
	if (str == "Sample_radius")
	{
    sem >> num;
    for (int i = 0; i < num; i++)
    {
      double temp;
      sem >> temp;
      //samples.vert[i].saved_radius = temp;
    }
	}

	sem >> str;
	if (str == "Skel_isVirtual")
	{
		sem >> num;
		bool temp;
		for (int i = 0; i < skeleton.branches.size(); i++)
		{
			for (int j = 0; j < skeleton.branches[i].curve.size(); j++)
			{
				sem >> temp;
				skeleton.branches[i].curve[j].is_skel_virtual = temp;
			}
		}
	}

  sem >> str;
  if (str == "Corresponding_sample_index")
  {
    sem >> num;
    int temp;
    for (int i = 0; i < skeleton.branches.size(); i++)
    {
      for (int j = 0; j < skeleton.branches[i].curve.size(); j++)
      {
        sem >> temp;
        skeleton.branches[i].curve[j].m_index = temp;
      }
    }
  }
  

	skeleton.generateBranchSampleMap();
}
