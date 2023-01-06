set(FOUNDATION_HEADER_LIST
	"${DRE_SOURCE_DIR}/include/foundation/Common.hpp"
	"${DRE_SOURCE_DIR}/include/foundation/class_features/ContiniousDataStorage.hpp"
	"${DRE_SOURCE_DIR}/include/foundation/class_features/NonCopyable.hpp"
	"${DRE_SOURCE_DIR}/include/foundation/class_features/NonMovable.hpp"
	"${DRE_SOURCE_DIR}/include/foundation/container/HashTable.hpp"
	"${DRE_SOURCE_DIR}/include/foundation/container/InplaceHashTable.hpp"
	"${DRE_SOURCE_DIR}/include/foundation/container/InplaceVector.hpp"
	"${DRE_SOURCE_DIR}/include/foundation/container/ObjectPool.hpp"
	"${DRE_SOURCE_DIR}/include/foundation/container/ObjectPoolQueue.hpp"
	"${DRE_SOURCE_DIR}/include/foundation/container/StackVector.hpp"
	"${DRE_SOURCE_DIR}/include/foundation/container/Vector.hpp"
	"${DRE_SOURCE_DIR}/include/foundation/container/Vector.hpp"
	"${DRE_SOURCE_DIR}/include/foundation/event/InplaceEvent.hpp"
	"${DRE_SOURCE_DIR}/include/foundation/input/InputSystem.hpp"
	"${DRE_SOURCE_DIR}/include/foundation/input/Keyboard.hpp"
	"${DRE_SOURCE_DIR}/include/foundation/math/SimpleMath.hpp"
	"${DRE_SOURCE_DIR}/include/foundation/memory/AllocatorBuddy.hpp"
	"${DRE_SOURCE_DIR}/include/foundation/memory/AllocatorLinear.hpp"
	"${DRE_SOURCE_DIR}/include/foundation/memory/AllocatorPool.hpp"
	"${DRE_SOURCE_DIR}/include/foundation/memory/AllocatorScopeStack.hpp"
	"${DRE_SOURCE_DIR}/include/foundation/memory/ByteBuffer.hpp"
	"${DRE_SOURCE_DIR}/include/foundation/memory/ElementAllocator.hpp"
	"${DRE_SOURCE_DIR}/include/foundation/memory/InplaceObjectAllocator.hpp"
	"${DRE_SOURCE_DIR}/include/foundation/memory/Memory.hpp"
	"${DRE_SOURCE_DIR}/include/foundation/memory/MemoryOps.hpp"
	"${DRE_SOURCE_DIR}/include/foundation/memory/Pointer.hpp"
	"${DRE_SOURCE_DIR}/include/foundation/string/ConstString.hpp"
	"${DRE_SOURCE_DIR}/include/foundation/string/InplaceString.hpp"
	"${DRE_SOURCE_DIR}/include/foundation/system/DynamicLibrary.hpp"
	"${DRE_SOURCE_DIR}/include/foundation/system/Window.hpp"
	"${DRE_SOURCE_DIR}/include/foundation/util/AlignedStorage.hpp"
	"${DRE_SOURCE_DIR}/include/foundation/util/Hash.hpp")
	
set(FOUNDATION_SOURCE_LIST
	"${DRE_SOURCE_DIR}/src/foundation/class_features/ContiniousDataStorage.cpp"
	"${DRE_SOURCE_DIR}/src/foundation/input/InputSystem.cpp"
	"${DRE_SOURCE_DIR}/src/foundation/input/Keyboard.cpp"
	"${DRE_SOURCE_DIR}/src/foundation/memory/AllocatorScopeStack.cpp"
	"${DRE_SOURCE_DIR}/src/foundation/memory/ByteBuffer.cpp"
	"${DRE_SOURCE_DIR}/src/foundation/memory/Memory.cpp"
	"${DRE_SOURCE_DIR}/src/foundation/system/DynamicLibrary.cpp"
	"${DRE_SOURCE_DIR}/src/foundation/system/Window.cpp"
	"${DRE_SOURCE_DIR}/src/foundation/util/Hash.cpp")
	
add_library(foundation STATIC 
	${FOUNDATION_HEADER_LIST} 
	${FOUNDATION_SOURCE_LIST})

target_include_directories(foundation PUBLIC "${DRE_SOURCE_DIR}/include")

target_compile_definitions(foundation PUBLIC WIN32_LEAN_AND_MEAN NOMINMAX)

target_compile_features(foundation PUBLIC cxx_std_17)
# TODO: disable exceptions

source_group(
	TREE "${DRE_SOURCE_DIR}/include"
	PREFIX "Header Files"
	FILES ${FOUNDATION_HEADER_LIST})