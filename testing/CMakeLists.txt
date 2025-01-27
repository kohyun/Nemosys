# adding gtest subdir with its own cmake file
if(WIN32)
  set(BUILD_SHARED_LIBS ON)
  add_definitions(-DGTEST_HAS_TR1_TUPLE=0)
endif()
set(BUILD_GMOCK OFF)
set(INSTALL_GTEST OFF)
if(ENABLE_OMEGAH_CUDA)
  set(gtest_disable_pthreads ON CACHE BOOL "" FORCE)
endif()
add_subdirectory(lib/gtest-1.8.1)

# include gtest library. gtest_SOURCE_DIR is availble from subdir addition
add_custom_target(unittests)

# Set location of test scripts, test data, and run directory
file(GLOB_RECURSE test_files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} test_data/*)
foreach(test_file ${test_files})
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${test_file} ${CMAKE_CURRENT_BINARY_DIR}/${test_file} COPYONLY)
endforeach()
set(TEST_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/test_scripts)
set(TEST_DATA ${CMAKE_CURRENT_BINARY_DIR}/test_data)

# Set environment variables for testing.
if(WIN32)
  # Note: Do not indent. The indentation whitespace is added to the variable!
  set(TEST_ENV_PATH_OPTIONS
      "PATH=${CMAKE_CURRENT_BINARY_DIR};\
${CMAKE_BINARY_DIR};\
${CMAKE_BINARY_DIR}/contrib/ann-1.1.2;\
${CMAKE_BINARY_DIR}/contrib/madlib-1.3.0;\
${CMAKE_BINARY_DIR}/contrib/omega_h-9.31.2/src;\
${CMAKE_BINARY_DIR}/contrib/kmeans-5b9691d;\
${CMAKE_BINARY_DIR}/contrib/smesh-9.6.0/Kernel;\
${CMAKE_BINARY_DIR}/contrib/smesh-9.6.0/Geom;\
${CMAKE_BINARY_DIR}/contrib/smesh-9.6.0/SMESH;\
${CMAKE_BINARY_DIR}/contrib/smesh-9.6.0/NETGENPlugin;\
${gtest_BINARY_DIR};\
${VTK_LIB_DIR};\
${GMSH_LIB_PATH};\
${NETGEN_LIBRARY_DIR};${NETGEN_LIBRARY_DIR}/../bin;\
${ZLIB_LIB_PATH};${ZLIB_LIB_PATH}/../bin;\
${METIS_LIB_PATH};\
${CGNS_LIB_PATH};${CGNS_LIB_PATH}/../bin;\
${HDF5_LIB_PATH};${HDF5_LIB_PATH}/../bin;\
${EXO_LIB_PATH};${EXO_LIB_PATH}/../bin;\
${netCDF_LIB_DIR};${netCDF_LIB_DIR}/../bin;\
$ENV{PATH}")
  # Escape the semicolons so set_tests_properties doesn't treat TEST_ENV_PATH_OPTIONS as a list
  string(REGEX REPLACE "\;" "\\\;" TEST_ENVIRONMENT_VARS "${TEST_ENV_PATH_OPTIONS}")
elseif(APPLE)
  set(TEST_ENV_PATH_OPTIONS "PATH=${CMAKE_CURRENT_BINARY_DIR}:$ENV{PATH}")
  # Note: Do not indent. The indentation whitespace is added to the variable!
  set(TEST_ENV_LD_OPTIONS
      "DYLD_FALLBACK_LIBRARY_PATH=${CMAKE_CURRENT_BINARY_DIR}:\
${CMAKE_BINARY_DIR}:\
${VTK_LIB_DIR}:\
${GMSH_LIB_PATH}:\
${NETGEN_LIBRARY_DIR}:\
${OpenCASCADE_LIBRARY_DIR}:\
$ENV{LD_LIBRARY_PATH}")
  set(TEST_ENVIRONMENT_VARS "${TEST_ENV_PATH_OPTIONS}" "${TEST_ENV_LD_OPTIONS}")
else()
  set(TEST_ENV_PATH_OPTIONS "PATH=${CMAKE_CURRENT_BINARY_DIR}:$ENV{PATH}")
  # Note: Do not indent. The indentation whitespace is added to the variable!
  set(TEST_ENV_LD_OPTIONS
      "LD_LIBRARY_PATH=${CMAKE_CURRENT_BINARY_DIR}:\
${CMAKE_BINARY_DIR}:\
${VTK_LIB_DIR}:\
${GMSH_LIB_PATH}:\
${NETGEN_LIBRARY_DIR}:\
${OpenCASCADE_LIBRARY_DIR}:\
${CGNS_LIB_PATH}:\
$ENV{LD_LIBRARY_PATH}")
  set(TEST_ENVIRONMENT_VARS "${TEST_ENV_PATH_OPTIONS}" "${TEST_ENV_LD_OPTIONS}")
endif()

message(STATUS "Test Env Options: ${TEST_ENVIRONMENT_VARS}")

#-------------------------------------------------------------------------------
# NEM_add_test_executable(<test_exe_name>)
#
# Adds a test executable named run<test_exe_name>Test. Requires:
#  ${TEST_SCRIPTS}/test<test_exe_name>.C  = The executable to turn into test
# Generates:
#  run<test_exe_name>Test                 = name of executable
macro(NEM_add_test_executable TEST_EXE_NAME)
  add_executable(run${TEST_EXE_NAME}Test ${TEST_SCRIPTS}/test${TEST_EXE_NAME}.C)
  target_link_libraries(run${TEST_EXE_NAME}Test gtest_main Nemosys)
  add_dependencies(unittests run${TEST_EXE_NAME}Test)
endmacro()
#-------------------------------------------------------------------------------

#-------------------------------------------------------------------------------
# NEM_add_test(<test_name> <test_exe_name> <test_pwd> [arguments...])
#
# Adds a test to Nemosys named <test_name>Test using run<test_exe_name>Test
# executable. Requires:
#  run<test_exe_name>Test         = name of executable
#                                   (use NEM_add_test_executable())
#  ${TEST_DATA}/<test_pwd>        = Directory to use as working directory
#                                   to run the test
#  [arguments...]                 = Arguments passed to executable
# Generates:
#  <test_name>Test                = name of CTest test
macro(NEM_add_test TEST_NAME TEST_EXE_NAME TEST_PWD)
  add_test(NAME ${TEST_NAME}Test
      COMMAND run${TEST_EXE_NAME}Test --gtest_output=xml:${CMAKE_CURRENT_BINARY_DIR}/junit/ ${ARGN}
      WORKING_DIRECTORY ${TEST_DATA}/${TEST_PWD}
      )
  set_tests_properties(${TEST_NAME}Test PROPERTIES ENVIRONMENT "${TEST_ENVIRONMENT_VARS}")
endmacro()
#-------------------------------------------------------------------------------

# Define test executables ######################################################

NEM_add_test_executable(Cubature)
NEM_add_test_executable(Conversion)
NEM_add_test_executable(CobaltGen)
NEM_add_test_executable(PatranGen)
NEM_add_test_executable(OrthoPoly)
NEM_add_test_executable(PatchRecovery)
NEM_add_test_executable(Transfer)
NEM_add_test_executable(MeshGen)
NEM_add_test_executable(PNTGen)
NEM_add_test_executable(AutoVerification)
NEM_add_test_executable(Refinement)
NEM_add_test_executable(ExoMesh)

NEM_add_test_executable(GeoMeshBase)
NEM_add_test_executable(GeoMeshFactory)

NEM_add_test_executable(ExoGeoMesh)
NEM_add_test_executable(InpGeoMesh)
NEM_add_test_executable(OshGeoMesh)
NEM_add_test_executable(VtkGeoMesh)

NEM_add_test_executable(SrvBase)

NEM_add_test_executable(OmegahRefineSrv)

NEM_add_test_executable(OmegahRefineDriver)

# custom-built tests
if(ENABLE_EPIC)
  NEM_add_test_executable(Ep16Pre)
  NEM_add_test_executable(Ep16Post)
endif()
if(ENABLE_CFMSH)
  NEM_add_test_executable(CFM)
  NEM_add_test_executable(CFMQual)
  NEM_add_test_executable(SHM)
  target_link_libraries(runSHMTest Boost::boost Boost::filesystem)
  NEM_add_test_executable(BlockMSH)
  NEM_add_test_executable(PackMesh)
  NEM_add_test_executable(FoamGeoMesh)
endif()
if(HAVE_HDF5 AND ENABLE_GMSH)
  NEM_add_test_executable(Proteus)
endif()
if(ENABLE_TEMPLATE_MESH)
  NEM_add_test_executable(TemplateMesh)
endif()
if(ENABLE_CONSRV_VOLUME_TRANSFER)
  NEM_add_test_executable(ConservativeVolumeTransfer)
endif()
if(ENABLE_CONSRV_SURFACE_TRANSFER)
  NEM_add_test_executable(ConservativeSurfaceTransfer)
endif()
if(ENABLE_OMEGAH_CUDA)
  NEM_add_test_executable(OshGeoMeshCUDA)
endif()
if(ENABLE_OPENMP)
  NEM_add_test_executable(Scaling)
endif()
if(ENABLE_GMSH)
  NEM_add_test_executable(RocPackPeriodic)
  NEM_add_test_executable(GmshMesh)
  NEM_add_test_executable(GmshGeoMesh)
  NEM_add_test_executable(GmshMeshGen)
endif()
if(ENABLE_OPENCASCADE)
  NEM_add_test_executable(NucMesh)
endif()

# Define tests #################################################################

NEM_add_test(cubatureInterp Cubature CubatureTest
    cube_refined.vtu
    cube_refinedGaussPoints.vtp
    cube_refinedGaussPointsNoData.vtp
    integrationREF.vtu
    single-hex.vtu
    )

NEM_add_test(conversion Conversion ConversionTest
    case0001_ref.vtu
    case0001.msh
    hinge.vtu
    hinge.vol
    legacyVTK1.vtk
    legacyVTK2.vtk
    legacyVTK1_ref.vtu
    legacyVTK2_ref.vtu
    gorilla.vtp
    pnt-tri.pntmesh
    pnt-tri-ref.vtu
    pnt-quad.pntmesh
    pnt-quad-ref.vtu
    pnt-hex.pntmesh
    pnt-hex-ref.vtu
    pnt-mix.pntmesh
    pnt-mix-ref.vtu
    geom_pack_mesh.vtu
    geom_pack_mesh_ref.vtu
    Building.vtu
    TetBuilding.vtu
    conversion_exo2exo.json
    )

NEM_add_test(cobaltGen CobaltGen CobaltGenTest
    vtk2cobalt.json
    fullfluid.cgr
    fullfluid_ref.cgr
    )

NEM_add_test(patranGen PatranGen PatranGenTest
    vtk2patran.json
    fullsolid.out
    fullsolid_ref.out
    )

NEM_add_test(orthoPoly OrthoPoly OrthoPolyTest F.txt)

NEM_add_test(patchRecovery PatchRecovery PatchRecoveryTest
    case0001.vtu
    testRef.vtu
    fixedWithData.vtu
    )

NEM_add_test(transfer Transfer TransferTest
    pointSource.vtu
    cellSource.vtu
    target.vtu
    pntRef.vtu
    cellRef.vtu
    )

NEM_add_test(meshGen MeshGen MeshGenTest
    default.json
    hingeRef.vtu
    unif.json
    hingeUnifRef.vtu
    geom.json
    hingeGeomRef.vtu
    )

NEM_add_test(refinement Refinement RefinementTest
    refine_value.json
    refined_beam_value.vtu
    gold_refined_beam_value.vtu
    refine_uniform.json
    refined_beam_uniform.vtu
    gold_refined_beam_uniform.vtu
    refine_AMR.json
    Airfoil_InputMesh.vtu
    fdeep_model.json
    )


NEM_add_test(autoVerif AutoVerification AutoVerificationTest
    finer.vtu
    fine.vtu
    coarse.vtu
    richardson.vtu
    auto_verification.json
    )

NEM_add_test(pntgen PNTGen PNTGenTest
    bench1.json bench1_conv_gold.pntmesh
    bench5.json bench5_conv_gold.pntmesh
    bench6.json bench6_conv_gold.pntmesh
    )

NEM_add_test(exoMesh ExoMesh ExoMeshTest
    hardenedUGbunker_soil2.g
    pack.g
    test_merge_in.g
    test_merge_ref.g
    four_boxes_tetra.e
    four_boxes_hexa.e
    simple_circles_test.e
    )

NEM_add_test(geoMeshBase GeoMeshBase "")
NEM_add_test(geoMeshFactory GeoMeshFactory GeoMeshFactoryTest
    vox8.vtu
    cube41.msh
    box0.osh
    smallTestFile.exo
    Hex_Cube_Read.foam
    )
NEM_add_test(vtkGeoMesh VtkGeoMesh VtkGeoMeshTest
    unstructured_grid.vtk
    structured_points.vtk
    vox8.vtu
    two_mesh.vtu
    )
NEM_add_test(inpGeoMesh InpGeoMesh InpGeoMeshTest
    panel_problem_1_gmsh_native_solid.msh
    SOLID_Plate.inp
    gold_SOLID_Plate.vtu
    )
NEM_add_test(exoGeoMesh ExoGeoMesh ExoGeoMeshTest
    hardenedUGbunker_soil2.g
    pack.g
    smallTestFile.exo
    hasPointData.vtu
    cube41.msh
    )
NEM_add_test(oshGeoMesh OshGeoMesh OshGeoMeshTest)

NEM_add_test(srvBase SrvBase "")
NEM_add_test(omegahRefineSrv OmegahRefineSrv OmegahRefineTest)

NEM_add_test(omegahRefineDriver OmegahRefineDriver OmegahRefineTest
    refine_value.json
    refined_beam_value.vtu
    gold_refined_beam_value.vtu
    refine_uniform.json
    refined_beam_uniform.vtu
    gold_refined_beam_uniform.vtu
    refine_value_hex.json
    refined_test_27_5_0.vtu
    refined_test_27_5_0.vtu
    )

if(ENABLE_OPENCASCADE)
  set(NucMesh_ARGUMENTS
      hex_array_pattern.json
      hex_array_pattern_REF.vtu
      circlesInPolys.json
      circlesInPolys_REF.vtu
      rectangular_array_pattern.json
      rectangular_array_pattern_REF.vtu
      cartesian_array.json
      cartesian_array_REF.vtu)
  if(ENABLE_NETGEN)
    list(APPEND NucMesh_ARGUMENTS
        simple_circles.json
        simple_circles_REF.vtu
        concentric_circles.json
        concentric_circles_REF.vtu
        concentric_circles_2.json
        concentric_circles_2_REF.vtu
        simple_polygons.json
        simple_polygons_REF.vtu
        concentric_polygons.json
        concentric_polygons_REF.vtu
        polar_array_rot_with_array.json
        polar_array_rot_with_array_REF.vtu
        N_mesh.json
        N_mesh_REF.vtu
        e_mesh.json
        e_mesh_REF.vtu
        threeD.json
        threeD_REF.exo)
  endif()
  NEM_add_test(nucMesh NucMesh NucMeshTest ${NucMesh_ARGUMENTS})
endif()

if(ENABLE_GMSH)
  NEM_add_test(rocPackPeriodic RocPackPeriodic rocPackPeriodic
      rocOut
      periodicGeom_ref.vtk
      )
  NEM_add_test(gmshMesh GmshMesh GmshMeshTest
    cube2.msh
    cube4.msh
    cube41.msh
    )
endif()

if(ENABLE_TEMPLATE_MESH)
  NEM_add_test(templateMesh TemplateMesh TemplateMeshTest
      test_spiral_tape.json
      spiral_test_REF.msh
      )
endif()

if(ENABLE_EPIC)
  NEM_add_test(ep16pre Ep16Pre ep16pre
      gmsh2exo.json bctest_ref.g
      ep16.json bctest_ref.dat
      )
  NEM_add_test(ep16post Ep16Post ep16post)
endif()

if(ENABLE_CFMSH)
  # cfMesh tests
  NEM_add_test(cfm_small CFM cfm_sm cfmsh.json)
  NEM_add_test(cfm_2d CFM cfm_2d cfmsh.json)
  NEM_add_test(cfm_hinge CFM cfm_hinge cfmsh.json)
  NEM_add_test(cfm_bunny CFM cfm_bunny cfmsh.json)
  NEM_add_test(cfm_bunny_oct CFM cfm_bunny_oct cfmsh.json)

  # cfMesh Quality test
  NEM_add_test(cfm_qual CFMQual cfm_qual cfmsh.json cfmsh_qual.json)

  if(ENABLE_GMSH)
    # PackMesh test
    NEM_add_test(packmesh PackMesh packmesh PackMsh_Master.json)
  endif()

  # snappyHeshMesh tests
  NEM_add_test(snappy_BoxPack SHM snappy_BoxPack shmsh.json)
  NEM_add_test(snappy_Bullet SHM snappy_Bullet shmsh.json)
  NEM_add_test(snappy_Flange SHM snappy_Flange shmsh.json)

  # BlockMesh tests
  NEM_add_test(blockMsh_Box BlockMSH blockMsh_Box blockMsh_Box.json)
  NEM_add_test(blockMsh_Sphere BlockMSH blockMsh_Sphere blockMsh_Sphere.json)
  NEM_add_test(blockMsh_TCone BlockMSH blockMsh_TCone blockMsh_TCone.json)

  # FoamGeoMesh tests
  NEM_add_test(foamGeoMesh FoamGeoMesh FoamGeoMeshTest
    Hex_Cube_Read
    Tet_Cube_Read
    Hex_Cube_Write
    Tet_Cube_Write
  )
endif()

if(ENABLE_CONSRV_VOLUME_TRANSFER)
  NEM_add_test(conservativeVolumeTransfer ConservativeVolumeTransfer
      ConservativeVolumeTransferTest
      sources/sph_source.vtk
      targets/inner_sphere.vtk)
endif()

if(ENABLE_CONSRV_SURFACE_TRANSFER)
  NEM_add_test(conservativeSurfaceTransfer ConservativeSurfaceTransfer
      ConservativeSurfaceTransferTest
      sources/plane_source.vtk
      targets/plane_target.vtk)
endif()

if(ENABLE_GMSH)
  NEM_add_test(gmshGeoMesh GmshGeoMesh GmshGeoMeshTest
    cube41.msh
    cube_UnnamedPhyGrp.msh
    cube_NamedPhyGrp.msh
    )

  NEM_add_test(gmshMeshGen GmshMeshGen gmshMeshGenTest
    box_test.json
    box_test_REF.vtu
    pitzdaily_test.json
    pitzdaily_test_REF.msh
  )
endif()

if(ENABLE_OMEGAH_CUDA)
  enable_language(CUDA)
  NEM_add_test(oshGeoMeshCUDA OshGeoMeshCUDA "")
  set_source_files_properties(${TEST_SCRIPTS}/testOshGeoMesh.C
      ${TEST_SCRIPTS}/testOshGeoMeshCUDA.C
      ${TEST_SCRIPTS}/testOmegahRefineSrv.C
      PROPERTIES LANGUAGE CUDA)
  target_link_libraries(runOshGeoMeshCUDATest nvidia-ml)
endif()

# Python wrapper testing
if(ENABLE_PYTHON_BINDINGS)
  find_package(PythonInterp 3 REQUIRED)
  if(NOT PYTHONINTERP_FOUND)
    message(STATUS "Python interpreter NOT found")
  else()
    message(STATUS "Python interpreter found")
    configure_file(test_scripts/test_pyNemosys.py ${CMAKE_CURRENT_BINARY_DIR}/../python/test_pyNemosys.py)
    add_test(NAME testPyNemosys
        COMMAND ${PYTHON_EXECUTABLE} setup.py test
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../python)
    set_tests_properties(testPyNemosys PROPERTIES ENVIRONMENT "${TEST_ENVIRONMENT_VARS}")
  endif()
endif()

if(HAVE_HDF5 AND ENABLE_GMSH)
  # Proteus testing
  NEM_add_test(proteus Proteus proteus
      bench02.json
      bench02_SN.vtu
      bench02_SN_gold.vtu
      bench02_SN.exo
      bench02_SN_gold.exo
      )
endif()

if(ENABLE_OPENMP)
#  NEM_add_test(scaling Scaling ScalingTest
#            fine.vtk finer.vtk finest.vtk)
endif()
