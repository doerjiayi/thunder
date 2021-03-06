// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: log.proto

#ifndef PROTOBUF_INCLUDED_log_2eproto
#define PROTOBUF_INCLUDED_log_2eproto

#include <string>

#include <google/protobuf/stubs/common.h>

#if GOOGLE_PROTOBUF_VERSION < 3006000
#error This file was generated by a newer version of protoc which is
#error incompatible with your Protocol Buffer headers.  Please update
#error your headers.
#endif
#if 3006000 < GOOGLE_PROTOBUF_MIN_PROTOC_VERSION
#error This file was generated by an older version of protoc which is
#error incompatible with your Protocol Buffer headers.  Please
#error regenerate this file with a newer version of protoc.
#endif

#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/arena.h>
#include <google/protobuf/arenastring.h>
#include <google/protobuf/generated_message_table_driven.h>
#include <google/protobuf/generated_message_util.h>
#include <google/protobuf/inlined_string_field.h>
#include <google/protobuf/metadata.h>
#include <google/protobuf/message.h>
#include <google/protobuf/repeated_field.h>  // IWYU pragma: export
#include <google/protobuf/extension_set.h>  // IWYU pragma: export
#include <google/protobuf/unknown_field_set.h>
// @@protoc_insertion_point(includes)
#define PROTOBUF_INTERNAL_EXPORT_protobuf_log_2eproto 

namespace protobuf_log_2eproto {
// Internal implementation detail -- do not use these members.
struct TableStruct {
  static const ::google::protobuf::internal::ParseTableField entries[];
  static const ::google::protobuf::internal::AuxillaryParseTableField aux[];
  static const ::google::protobuf::internal::ParseTable schema[2];
  static const ::google::protobuf::internal::FieldMetadata field_metadata[];
  static const ::google::protobuf::internal::SerializationTable serialization_table[];
  static const ::google::protobuf::uint32 offsets[];
};
void AddDescriptors();
}  // namespace protobuf_log_2eproto
namespace logqueue {
class log;
class logDefaultTypeInternal;
extern logDefaultTypeInternal _log_default_instance_;
class log_ack;
class log_ackDefaultTypeInternal;
extern log_ackDefaultTypeInternal _log_ack_default_instance_;
}  // namespace logqueue
namespace google {
namespace protobuf {
template<> ::logqueue::log* Arena::CreateMaybeMessage<::logqueue::log>(Arena*);
template<> ::logqueue::log_ack* Arena::CreateMaybeMessage<::logqueue::log_ack>(Arena*);
}  // namespace protobuf
}  // namespace google
namespace logqueue {

// ===================================================================

class log : public ::google::protobuf::Message /* @@protoc_insertion_point(class_definition:logqueue.log) */ {
 public:
  log();
  virtual ~log();

  log(const log& from);

  inline log& operator=(const log& from) {
    CopyFrom(from);
    return *this;
  }
  #if LANG_CXX11
  log(log&& from) noexcept
    : log() {
    *this = ::std::move(from);
  }

  inline log& operator=(log&& from) noexcept {
    if (GetArenaNoVirtual() == from.GetArenaNoVirtual()) {
      if (this != &from) InternalSwap(&from);
    } else {
      CopyFrom(from);
    }
    return *this;
  }
  #endif
  static const ::google::protobuf::Descriptor* descriptor();
  static const log& default_instance();

  static void InitAsDefaultInstance();  // FOR INTERNAL USE ONLY
  static inline const log* internal_default_instance() {
    return reinterpret_cast<const log*>(
               &_log_default_instance_);
  }
  static constexpr int kIndexInFileMessages =
    0;

  void Swap(log* other);
  friend void swap(log& a, log& b) {
    a.Swap(&b);
  }

  // implements Message ----------------------------------------------

  inline log* New() const final {
    return CreateMaybeMessage<log>(NULL);
  }

  log* New(::google::protobuf::Arena* arena) const final {
    return CreateMaybeMessage<log>(arena);
  }
  void CopyFrom(const ::google::protobuf::Message& from) final;
  void MergeFrom(const ::google::protobuf::Message& from) final;
  void CopyFrom(const log& from);
  void MergeFrom(const log& from);
  void Clear() final;
  bool IsInitialized() const final;

  size_t ByteSizeLong() const final;
  bool MergePartialFromCodedStream(
      ::google::protobuf::io::CodedInputStream* input) final;
  void SerializeWithCachedSizes(
      ::google::protobuf::io::CodedOutputStream* output) const final;
  ::google::protobuf::uint8* InternalSerializeWithCachedSizesToArray(
      bool deterministic, ::google::protobuf::uint8* target) const final;
  int GetCachedSize() const final { return _cached_size_.Get(); }

  private:
  void SharedCtor();
  void SharedDtor();
  void SetCachedSize(int size) const final;
  void InternalSwap(log* other);
  private:
  inline ::google::protobuf::Arena* GetArenaNoVirtual() const {
    return NULL;
  }
  inline void* MaybeArenaPtr() const {
    return NULL;
  }
  public:

  ::google::protobuf::Metadata GetMetadata() const final;

  // nested types ----------------------------------------------------

  // accessors -------------------------------------------------------

  // string type = 1;
  void clear_type();
  static const int kTypeFieldNumber = 1;
  const ::std::string& type() const;
  void set_type(const ::std::string& value);
  #if LANG_CXX11
  void set_type(::std::string&& value);
  #endif
  void set_type(const char* value);
  void set_type(const char* value, size_t size);
  ::std::string* mutable_type();
  ::std::string* release_type();
  void set_allocated_type(::std::string* type);

  // string log_info = 2;
  void clear_log_info();
  static const int kLogInfoFieldNumber = 2;
  const ::std::string& log_info() const;
  void set_log_info(const ::std::string& value);
  #if LANG_CXX11
  void set_log_info(::std::string&& value);
  #endif
  void set_log_info(const char* value);
  void set_log_info(const char* value, size_t size);
  ::std::string* mutable_log_info();
  ::std::string* release_log_info();
  void set_allocated_log_info(::std::string* log_info);

  // @@protoc_insertion_point(class_scope:logqueue.log)
 private:

  ::google::protobuf::internal::InternalMetadataWithArena _internal_metadata_;
  ::google::protobuf::internal::ArenaStringPtr type_;
  ::google::protobuf::internal::ArenaStringPtr log_info_;
  mutable ::google::protobuf::internal::CachedSize _cached_size_;
  friend struct ::protobuf_log_2eproto::TableStruct;
};
// -------------------------------------------------------------------

class log_ack : public ::google::protobuf::Message /* @@protoc_insertion_point(class_definition:logqueue.log_ack) */ {
 public:
  log_ack();
  virtual ~log_ack();

  log_ack(const log_ack& from);

  inline log_ack& operator=(const log_ack& from) {
    CopyFrom(from);
    return *this;
  }
  #if LANG_CXX11
  log_ack(log_ack&& from) noexcept
    : log_ack() {
    *this = ::std::move(from);
  }

  inline log_ack& operator=(log_ack&& from) noexcept {
    if (GetArenaNoVirtual() == from.GetArenaNoVirtual()) {
      if (this != &from) InternalSwap(&from);
    } else {
      CopyFrom(from);
    }
    return *this;
  }
  #endif
  static const ::google::protobuf::Descriptor* descriptor();
  static const log_ack& default_instance();

  static void InitAsDefaultInstance();  // FOR INTERNAL USE ONLY
  static inline const log_ack* internal_default_instance() {
    return reinterpret_cast<const log_ack*>(
               &_log_ack_default_instance_);
  }
  static constexpr int kIndexInFileMessages =
    1;

  void Swap(log_ack* other);
  friend void swap(log_ack& a, log_ack& b) {
    a.Swap(&b);
  }

  // implements Message ----------------------------------------------

  inline log_ack* New() const final {
    return CreateMaybeMessage<log_ack>(NULL);
  }

  log_ack* New(::google::protobuf::Arena* arena) const final {
    return CreateMaybeMessage<log_ack>(arena);
  }
  void CopyFrom(const ::google::protobuf::Message& from) final;
  void MergeFrom(const ::google::protobuf::Message& from) final;
  void CopyFrom(const log_ack& from);
  void MergeFrom(const log_ack& from);
  void Clear() final;
  bool IsInitialized() const final;

  size_t ByteSizeLong() const final;
  bool MergePartialFromCodedStream(
      ::google::protobuf::io::CodedInputStream* input) final;
  void SerializeWithCachedSizes(
      ::google::protobuf::io::CodedOutputStream* output) const final;
  ::google::protobuf::uint8* InternalSerializeWithCachedSizesToArray(
      bool deterministic, ::google::protobuf::uint8* target) const final;
  int GetCachedSize() const final { return _cached_size_.Get(); }

  private:
  void SharedCtor();
  void SharedDtor();
  void SetCachedSize(int size) const final;
  void InternalSwap(log_ack* other);
  private:
  inline ::google::protobuf::Arena* GetArenaNoVirtual() const {
    return NULL;
  }
  inline void* MaybeArenaPtr() const {
    return NULL;
  }
  public:

  ::google::protobuf::Metadata GetMetadata() const final;

  // nested types ----------------------------------------------------

  // accessors -------------------------------------------------------

  // string msg = 2;
  void clear_msg();
  static const int kMsgFieldNumber = 2;
  const ::std::string& msg() const;
  void set_msg(const ::std::string& value);
  #if LANG_CXX11
  void set_msg(::std::string&& value);
  #endif
  void set_msg(const char* value);
  void set_msg(const char* value, size_t size);
  ::std::string* mutable_msg();
  ::std::string* release_msg();
  void set_allocated_msg(::std::string* msg);

  // uint32 code = 1;
  void clear_code();
  static const int kCodeFieldNumber = 1;
  ::google::protobuf::uint32 code() const;
  void set_code(::google::protobuf::uint32 value);

  // @@protoc_insertion_point(class_scope:logqueue.log_ack)
 private:

  ::google::protobuf::internal::InternalMetadataWithArena _internal_metadata_;
  ::google::protobuf::internal::ArenaStringPtr msg_;
  ::google::protobuf::uint32 code_;
  mutable ::google::protobuf::internal::CachedSize _cached_size_;
  friend struct ::protobuf_log_2eproto::TableStruct;
};
// ===================================================================


// ===================================================================

#ifdef __GNUC__
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif  // __GNUC__
// log

// string type = 1;
inline void log::clear_type() {
  type_.ClearToEmptyNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
inline const ::std::string& log::type() const {
  // @@protoc_insertion_point(field_get:logqueue.log.type)
  return type_.GetNoArena();
}
inline void log::set_type(const ::std::string& value) {
  
  type_.SetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), value);
  // @@protoc_insertion_point(field_set:logqueue.log.type)
}
#if LANG_CXX11
inline void log::set_type(::std::string&& value) {
  
  type_.SetNoArena(
    &::google::protobuf::internal::GetEmptyStringAlreadyInited(), ::std::move(value));
  // @@protoc_insertion_point(field_set_rvalue:logqueue.log.type)
}
#endif
inline void log::set_type(const char* value) {
  GOOGLE_DCHECK(value != NULL);
  
  type_.SetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), ::std::string(value));
  // @@protoc_insertion_point(field_set_char:logqueue.log.type)
}
inline void log::set_type(const char* value, size_t size) {
  
  type_.SetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(),
      ::std::string(reinterpret_cast<const char*>(value), size));
  // @@protoc_insertion_point(field_set_pointer:logqueue.log.type)
}
inline ::std::string* log::mutable_type() {
  
  // @@protoc_insertion_point(field_mutable:logqueue.log.type)
  return type_.MutableNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
inline ::std::string* log::release_type() {
  // @@protoc_insertion_point(field_release:logqueue.log.type)
  
  return type_.ReleaseNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
inline void log::set_allocated_type(::std::string* type) {
  if (type != NULL) {
    
  } else {
    
  }
  type_.SetAllocatedNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), type);
  // @@protoc_insertion_point(field_set_allocated:logqueue.log.type)
}

// string log_info = 2;
inline void log::clear_log_info() {
  log_info_.ClearToEmptyNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
inline const ::std::string& log::log_info() const {
  // @@protoc_insertion_point(field_get:logqueue.log.log_info)
  return log_info_.GetNoArena();
}
inline void log::set_log_info(const ::std::string& value) {
  
  log_info_.SetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), value);
  // @@protoc_insertion_point(field_set:logqueue.log.log_info)
}
#if LANG_CXX11
inline void log::set_log_info(::std::string&& value) {
  
  log_info_.SetNoArena(
    &::google::protobuf::internal::GetEmptyStringAlreadyInited(), ::std::move(value));
  // @@protoc_insertion_point(field_set_rvalue:logqueue.log.log_info)
}
#endif
inline void log::set_log_info(const char* value) {
  GOOGLE_DCHECK(value != NULL);
  
  log_info_.SetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), ::std::string(value));
  // @@protoc_insertion_point(field_set_char:logqueue.log.log_info)
}
inline void log::set_log_info(const char* value, size_t size) {
  
  log_info_.SetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(),
      ::std::string(reinterpret_cast<const char*>(value), size));
  // @@protoc_insertion_point(field_set_pointer:logqueue.log.log_info)
}
inline ::std::string* log::mutable_log_info() {
  
  // @@protoc_insertion_point(field_mutable:logqueue.log.log_info)
  return log_info_.MutableNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
inline ::std::string* log::release_log_info() {
  // @@protoc_insertion_point(field_release:logqueue.log.log_info)
  
  return log_info_.ReleaseNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
inline void log::set_allocated_log_info(::std::string* log_info) {
  if (log_info != NULL) {
    
  } else {
    
  }
  log_info_.SetAllocatedNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), log_info);
  // @@protoc_insertion_point(field_set_allocated:logqueue.log.log_info)
}

// -------------------------------------------------------------------

// log_ack

// uint32 code = 1;
inline void log_ack::clear_code() {
  code_ = 0u;
}
inline ::google::protobuf::uint32 log_ack::code() const {
  // @@protoc_insertion_point(field_get:logqueue.log_ack.code)
  return code_;
}
inline void log_ack::set_code(::google::protobuf::uint32 value) {
  
  code_ = value;
  // @@protoc_insertion_point(field_set:logqueue.log_ack.code)
}

// string msg = 2;
inline void log_ack::clear_msg() {
  msg_.ClearToEmptyNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
inline const ::std::string& log_ack::msg() const {
  // @@protoc_insertion_point(field_get:logqueue.log_ack.msg)
  return msg_.GetNoArena();
}
inline void log_ack::set_msg(const ::std::string& value) {
  
  msg_.SetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), value);
  // @@protoc_insertion_point(field_set:logqueue.log_ack.msg)
}
#if LANG_CXX11
inline void log_ack::set_msg(::std::string&& value) {
  
  msg_.SetNoArena(
    &::google::protobuf::internal::GetEmptyStringAlreadyInited(), ::std::move(value));
  // @@protoc_insertion_point(field_set_rvalue:logqueue.log_ack.msg)
}
#endif
inline void log_ack::set_msg(const char* value) {
  GOOGLE_DCHECK(value != NULL);
  
  msg_.SetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), ::std::string(value));
  // @@protoc_insertion_point(field_set_char:logqueue.log_ack.msg)
}
inline void log_ack::set_msg(const char* value, size_t size) {
  
  msg_.SetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(),
      ::std::string(reinterpret_cast<const char*>(value), size));
  // @@protoc_insertion_point(field_set_pointer:logqueue.log_ack.msg)
}
inline ::std::string* log_ack::mutable_msg() {
  
  // @@protoc_insertion_point(field_mutable:logqueue.log_ack.msg)
  return msg_.MutableNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
inline ::std::string* log_ack::release_msg() {
  // @@protoc_insertion_point(field_release:logqueue.log_ack.msg)
  
  return msg_.ReleaseNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
inline void log_ack::set_allocated_msg(::std::string* msg) {
  if (msg != NULL) {
    
  } else {
    
  }
  msg_.SetAllocatedNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), msg);
  // @@protoc_insertion_point(field_set_allocated:logqueue.log_ack.msg)
}

#ifdef __GNUC__
  #pragma GCC diagnostic pop
#endif  // __GNUC__
// -------------------------------------------------------------------


// @@protoc_insertion_point(namespace_scope)

}  // namespace logqueue

// @@protoc_insertion_point(global_scope)

#endif  // PROTOBUF_INCLUDED_log_2eproto
