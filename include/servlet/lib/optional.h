/*
Copyright (c) 2016 Alexei Novakov
https://github.com/novalexei

Distributed under the Boost Software License, Version 1.0.
http://boost.org/LICENSE_1_0.txt
*/
#ifndef SERVLET_OPTIONAL_H
#define SERVLET_OPTIONAL_H

/**
 * @file optional.h
 * @brief Defines optional container objects and related methods.
 */

#include <utility>
#include <type_traits>

#include <servlet/lib/exception.h>

namespace servlet
{

/**
 * Class implements smart pointer with optional ownership.
 *
 * If the object owns the contained pointer it will delete the pointer
 * on destruction, otherwise the contained pointer will not be deleted.
 *
 * @tparam the type of the value to manage.
 */
template<typename T>
class optional_ptr
{
public:
    /**
     * Type definition for value type
     */
    typedef T value_type;
    /**
     * Type definition for pointer type
     */
    typedef T *pointer;
    /**
     * Type definition for const pointer type
     */
    typedef const T *const_pointer;

    /**
     * Default constructor.
     *
     * Creates empty object with <code>nullptr</code> as the
     * contained pointer.
     */
    constexpr optional_ptr() {}
    /**
     * Constructor which creates the object with a given pointer and
     * specified ownership.
     *
     * @param ptr Pointer to contain in optional_ptr
     * @param owner Ownership flag.
     */
    constexpr optional_ptr(T* ptr, bool owner = false) : _ptr{ptr}, _owner{owner} {}

    /**
     * Copy contructor.
     * @param other object to copy.
     */
    constexpr optional_ptr(const optional_ptr &other) = delete;
    /**
     * Move constructor
     * @param other object to move.
     */
    constexpr optional_ptr(optional_ptr &&other) : _ptr{other._ptr}, _owner{other._owner}
    { other._ptr = nullptr; other._owner = false; }

    /**
     * Destructor.
     *
     * If the object contains the valid pointer and is the owner of it,
     * deletes this pointer. Otherwise, does nothing.
     */
    ~optional_ptr() noexcept { if (_owner) delete _ptr; }

    /**
     * Clears the object.
     *
     * This method will delete the contained pointer if it is an owner,
     * drop the * pointer to <code>nullptr</code> and reset the ownership
     * flag to <code>false</code>
     */
    constexpr void clear()
    {
        if (_owner) delete _ptr;
        _ptr = nullptr; _owner = false;
    }

    /**
     * Copy is prohibited.
     */
    constexpr optional_ptr& operator=(const optional_ptr &other) = delete;
    /**
     * Move assignment.
     *
     * If this object contains valid pointer and is the owner it will delete
     * it before the assignment.
     *
     * @param other the object to move
     * @return self
     */
    constexpr optional_ptr& operator=(optional_ptr &&other) noexcept
    {
        if (_owner) delete _ptr;
        _ptr = other._ptr; _owner = other._owner;
        other._ptr = nullptr; other._owner = false;
        return *this;
    }

    /**
     * Assigns new pointer to this object.
     *
     * If this object already contains a valid pointer and is the owner it will
     * delelete it before the assignment.
     *
     * @param ptr new pointer to assign
     * @param owner ownership flag
     * @return self
     */
    constexpr optional_ptr& assign(T* ptr, bool owner = false)
    {
        if (_owner) delete _ptr;
        _ptr = ptr; _owner = owner;
        return *this;
    }

    /**
     * Checks whether this object contains valid pointer.
     * @return <code>true</code> if this object contains valid pointer,
     *         <code>false</code> otherwise.
     * @see #has_value
     */
    constexpr explicit operator bool() const noexcept { return _ptr != nullptr; }
    /**
     * Checks whether this object contains valid pointer.
     * @return <code>true</code> if this object contains valid pointer,
     *         <code>false</code> otherwise.
     */
    constexpr bool has_value() const noexcept { return _ptr != nullptr; }
    /**
     * Checks whether this object is the owner of the contained pointer.
     * @return <code>true</code> if this object is the owner of the contained
     *         pointer, <code>false</code> otherwise.
     */
    constexpr bool is_owner() const noexcept { return _owner; }

    /**
     * Accesses the contained pointer
     * @return contained pointer.
     * @throws null_pointer_exception if the contained pointer is <code>nullptr</code>.
     */
    constexpr T *operator->() { return _ptr ? _ptr : throw null_pointer_exception{"pointer is NULL"}; }
    /**
     * const version of pointer accessor
     * @return contained const pointer.
     * @throws null_pointer_exception if the contained pointer is <code>nullptr</code>.
     */
    constexpr const T *operator->() const
    { return _ptr ? _ptr : throw null_pointer_exception{"pointer is NULL"}; }

    /**
     * Accesses the contained pointer.
     * @return reference to contained value.
     * @see #has_value
     * @throws null_pointer_exception if the contained pointer is <code>nullptr</code>.
     */
    constexpr T& operator*() & { if (!_ptr) throw null_pointer_exception{"pointer is NULL"}; return *_ptr; }
    /**
     * Accesses the contained pointer
     * @return const reference to contained value.
     * @see #has_value
     * @throws null_pointer_exception if the contained pointer is <code>nullptr</code>.
     */
    constexpr const T& operator*() const& { if (!_ptr) throw null_pointer_exception{"pointer is NULL"}; return *_ptr; }

    /**
     * Swaps the contents with those of other.
     * @param other the object to exchange the contents with
     */
    constexpr void swap(optional_ptr &other) noexcept
    {
        T *tmp_ptr = _ptr;
        _ptr = other._ptr;
        other._ptr = tmp_ptr;
        bool tmp_owner = _owner;
        _owner = other._owner;
        other._owner = tmp_owner;
    }
    /**
     * If this object contains a valid pointer, delete that. Otherwise, there are
     * no effects.
     * @see #clear
     */
    constexpr void reset() noexcept { clear(); }

private:
    bool _owner = false;
    T* _ptr = nullptr;
};

/**
 * Optional reference implementation.
 *
 * It is resambles std::optional but doesn't copy the object, instead it stores the
 * reference to the object and never attempts to destroy it. This object might be
 * usefull when returning the object from container and it is not known up front
 * if this object exists.
 *
 * @tparam the type of the value to manage.
 */
template<typename T>
class optional_ref
{
    T *_ptr = nullptr;
public:
    /**
     * Type definition for value type
     */
    typedef T value_type;
    /**
     * Type definition for reference type
     */
    typedef T &reference;
    /**
     * Type definition for constant reference type
     */
    typedef const T &const_reference;

    /**
     * Default constructor.
     *
     * Constructs the object that does not contain a reference to any value.
     */
    constexpr optional_ref() noexcept {}
    /**
     * Constructs object that contains a reference to a given object,
     * @param obj Object to whcih this object will refere.
     */
    constexpr optional_ref(reference obj) noexcept : _ptr{&obj} {}

    /**
     * Copy constructor.
     * @param other object to copy.
     */
    constexpr optional_ref(const optional_ref &other) noexcept : _ptr{other._ptr} {}

    /**
     * Destructor.
     *
     * Does nothing.
     */
    ~optional_ref() noexcept {}

    /**
     * Replaces contents of this object with the contents of other
     * @param other object to assign
     * @return self
     */
    constexpr optional_ref &operator=(const optional_ref &other) noexcept { _ptr = other._ptr; return *this; }
    /**
     * Replaces contents of this object with the reference to other
     * @param obj object to refere
     * @return self
     */
    constexpr optional_ref &operator=(reference &obj) noexcept { *_ptr = obj; return *this; }

    /**
     * Accesses the contained value as a pointer.
     * @return a pointer to the contained value.
     * @throws null_pointer_exception if the contained pointer is <code>nullptr</code>.
     */
    constexpr T *operator->() { return _ptr ? _ptr : throw null_pointer_exception{"pointer is NULL"}; }
    /**
     * Accesses the contained value as a constant pointer.
     * @return a const pointer to the contained value.
     * @throws null_pointer_exception if the contained pointer is <code>nullptr</code>.
     */
    constexpr const T *operator->() const { return _ptr ? _ptr : throw null_pointer_exception{"pointer is NULL"}; }

    /**
     * Accesses the contained value as a reference.
     * @return a reference to the contained value.
     * @throws null_pointer_exception if the contained pointer is <code>nullptr</code>.
     */
    constexpr T& operator*() & { if (!_ptr) throw null_pointer_exception{"pointer is NULL"}; return*_ptr; }
    /**
     * Accesses the contained value as a constant reference.
     * @return a const reference to the contained value.
     * @throws null_pointer_exception if the contained pointer is <code>nullptr</code>.
     */
    constexpr const T& operator*() const& { if (!_ptr) throw null_pointer_exception{"pointer is NULL"}; return*_ptr; }
    /**
     * Accesses the contained value as a movable reference.
     * @return a movable reference to the contained value.
     * @throws null_pointer_exception if the contained pointer is <code>nullptr</code>.
     */
    constexpr T&& operator*() &&
    { if (!_ptr) throw null_pointer_exception{"pointer is NULL"}; return std::move(*_ptr); }
    /**
     * Accesses the contained value as a movable reference.
     * @return a movable reference to the contained value.
     * @throws null_pointer_exception if the contained pointer is <code>nullptr</code>.
     */
    constexpr const T&& operator*() const&&
    { if (!_ptr) throw null_pointer_exception{"pointer is NULL"}; return std::move(*_ptr); }

    /**
     * Checks whether this object contains valid pointer.
     * @return <code>true</code> if this object contains valid pointer,
     *         <code>false</code> otherwise.
     * @see #has_value
     */
    constexpr explicit operator bool() const noexcept { return _ptr != nullptr; }
    /**
     * Checks whether this object contains valid pointer.
     * @return <code>true</code> if this object contains valid pointer,
     *         <code>false</code> otherwise.
     */
    constexpr bool has_value() const noexcept { return _ptr != nullptr; }

    /**
     * Accesses the contained value as a reference.
     * @return a reference to the contained value.
     * @throws null_pointer_exception if the contained pointer is <code>nullptr</code>.
     */
    constexpr T& value() & { return _ptr ? *_ptr : throw null_pointer_exception{"pointer is NULL"}; }
    /**
     * Accesses the contained value as a constant reference.
     * @return a const reference to the contained value.
     * @throws null_pointer_exception if the contained pointer is <code>nullptr</code>.
     */
    constexpr const T& value() const& { return _ptr ? *_ptr : throw null_pointer_exception{"pointer is NULL"}; }
    /**
     * Accesses the contained value as a movable reference.
     * @return a movable reference to the contained value.
     * @throws null_pointer_exception if the contained pointer is <code>nullptr</code>.
     */
    constexpr T&& value() && { return _ptr ? std::move(*_ptr) : throw null_pointer_exception{"pointer is NULL"}; }
    /**
     * Accesses the contained value as a movable reference.
     * @return a movable reference to the contained value.
     * @throws null_pointer_exception if the contained pointer is <code>nullptr</code>.
     */
    constexpr const T&& value() const&&
    { return _ptr ? std::move(*_ptr) : throw null_pointer_exception{"pointer is NULL"}; }

    /**
     * Returns the contained value if this has a value, otherwise returns given value
     * @param dflt the value to use in case this is empty
     * @return The current value if this has a value, or dflt otherwise.
     */
    template<typename OT>
    constexpr T value_or(OT&& dflt) const& noexcept
    {
        static_assert(std::is_copy_constructible<T>::value, "result is not copy constructable");
        static_assert(std::is_convertible<OT&&, T>::value, "default is not convertible to result");
        return _ptr == nullptr ? static_cast<T>(std::forward(dflt)) : *_ptr;
    }
    /**
     * Returns the contained value if this has a value, otherwise returns given value
     * @param dflt the value to use in case this is empty
     * @return The current value if this has a value, or dflt otherwise.
     */
    template<typename OT>
    constexpr T value_or(OT&& dflt) && noexcept
    {
        static_assert(std::is_copy_constructible<T>::value, "result is not copy constructable");
        static_assert(std::is_convertible<OT&&, T>::value, "default is not convertible to result");
        return _ptr == nullptr ? static_cast<T>(std::forward(dflt)) : std::move(*_ptr);
    }

    /**
     * Swaps the contents with those of other.
     * @param other the object to exchange the contents with
     */
    constexpr void swap(optional_ref &other) noexcept
    {
        T *tmp = _ptr;
        _ptr = other._ptr;
        other._ptr = tmp;
    }
    /**
     * If this object contains a reference to an object, reset the pointer to
     * that to <code>nullptr</code>. Otherwise, there are no effects.
     */
    constexpr void reset() noexcept { _ptr = nullptr; }
};

/**
 * Stream output operator for optional_ptr object.
 *
 * If the object does not contain valid pointer "NULL" string will
 * be sent to output stream.
 * @param out stream to print to
 * @param opt Object to print
 * @return reference to output stream
 */
template<typename CharT, typename Traits, typename T>
std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& out, const optional_ptr<T>& opt)
{
    if (opt) out << *opt;
    else out << "NULL";
    return out;
}
/**
 * Stream output operator for optional_ref object.
 *
 * If the object does not contain valid reference "NULL" string will
 * be sent to output stream.
 * @param out stream to print to.
 * @param opt Object to print
 * @return reference to output stream
 */
template<typename CharT, typename Traits, typename T>
std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& out, const optional_ref<T>& opt)
{
    if (opt) out << *opt;
    else out << "NULL";
    return out;
}

/**
 * Performs "==" comparison of two optional_ref objects.
 *
 * @param l first object to compare
 * @param r second object to compare
 * @return <code>true</code> if contained objects are equal, or
 *         <code>false</code> otherwise.
 */
template<typename T>
constexpr bool operator==(const optional_ref<T> &l, const optional_ref<T> &r)
{
    return static_cast<bool>(l) == static_cast<bool>(r) && (!l || *l == *r);
}
/**
 * Performs "!=" comparison of two optional_ref objects.
 *
 * @param l first object to compare
 * @param r second object to compare
 * @return <code>true</code> if contained objects are not equal, or
 *         <code>false</code> otherwise.
 */
template<typename T>
constexpr bool operator!=(const optional_ref<T> &l, const optional_ref<T> &r) { return !(l == r); }

/**
 * Performs "<" comparison of two optional_ref objects.
 *
 * @param l first object to compare
 * @param r second object to compare
 * @return <code>true</code> if contained object in l is less than the
 *         contained object in r, or <code>false</code> otherwise.
 */
template<typename T>
constexpr bool operator<(const optional_ref<T> &l, const optional_ref<T> &r)
{
    return static_cast<bool>(r) && (!l || *l < *r);
}

/**
 * Performs ">" comparison of two optional_ref objects.
 *
 * @param l first object to compare
 * @param r second object to compare
 * @return <code>true</code> if contained object in l is greater than the
 *         contained object in r, or <code>false</code> otherwise.
 */
template<typename T>
constexpr bool operator>(const optional_ref<T> &l, const optional_ref<T> &r) { return r < l; }

/**
 * Performs "<=" comparison of two optional_ref objects.
 *
 * @param l first object to compare
 * @param r second object to compare
 * @return <code>true</code> if contained object in l is less or equal the
 *         contained object in r, or <code>false</code> otherwise.
 */
template<typename T>
constexpr bool operator<=(const optional_ref<T> &l, const optional_ref<T> &r) { return !(r < l); }

/**
 * Performs ">=" comparison of two optional_ref objects.
 *
 * @param l first object to compare
 * @param r second object to compare
 * @return <code>true</code> if contained object in l is greater or equal the
 *         contained object in r, or <code>false</code> otherwise.
 */
template<typename T>
constexpr bool operator>=(const optional_ref<T> &l, const optional_ref<T> &r) { return !(l < r); }

/**
 * Performs "==" comparison of optional_ref object and other object
 * which can be compared to the object contained in optional_ref.
 *
 * @param l optional_ref object to compare
 * @param r object to compare
 * @return <code>true</code> if contained object of optional_ref equals
 *         to r object, or <code>false</code> otherwise.
 */
template<typename T, typename U>
constexpr bool operator==(const optional_ref<T> &l, const U &r) { return l && *l == r; }

/**
 * Performs "==" comparison of optional_ref object and other object
 * which can be compared to the object contained in optional_ref.
 *
 * @param l object to compare
 * @param r optional_ref object to compare
 * @return <code>true</code> if contained object of optional_ref equals
 *         to l object, or <code>false</code> otherwise.
 */
template<typename T, typename U>
constexpr bool operator==(const T &l, const optional_ref<T> &r) { return r && l == *r; }

/**
 * Performs "!=" comparison of optional_ref object and other object
 * which can be compared to the object contained in optional_ref.
 *
 * @param l optional_ref object to compare
 * @param r object to compare
 * @return <code>true</code> if contained object of optional_ref not equals
 *         to r object, or <code>false</code> otherwise.
 */
template<typename T, typename U>
constexpr bool operator!=(const optional_ref<T> &l, U const &r) { return !l || !(*l == r); }

/**
 * Performs "!=" comparison of optional_ref object and other object
 * which can be compared to the object contained in optional_ref.
 *
 * @param l object to compare
 * @param r optional_ref object to compare
 * @return <code>true</code> if contained object of optional_ref not equals
 *         to l object, or <code>false</code> otherwise.
 */
template<typename T, typename U>
constexpr bool operator!=(const U &l, const optional_ref<T> &r) { return !r || !(l == *r); }

/**
 * Performs "<" comparison of optional_ref object and other object
 * which can be compared to the object contained in optional_ref.
 *
 * @param l optional_ref object to compare
 * @param r object to compare
 * @return <code>true</code> if contained object of optional_ref less than
 *         other object, or <code>false</code> otherwise.
 */
template<typename T, typename U>
constexpr bool operator<(const optional_ref<T> &l, const U &r) { return !l || *l < r; }

/**
 * Performs "<" comparison of an object and another object contained
 * in optional_ref.
 *
 * @param l object to compare
 * @param r optional_ref object to compare
 * @return <code>true</code> if l object is less than the object contained in
 *         optional_ref parameter, or <code>false</code> otherwise.
 */
template<typename T, typename U>
constexpr bool operator<(const U &l, const optional_ref<T> &r) { return r && l < *r; }

/**
 * Performs ">" comparison of optional_ref object and other object
 * which can be compared to the object contained in optional_ref.
 *
 * @param l optional_ref object to compare
 * @param r object to compare
 * @return <code>true</code> if contained object of optional_ref greater than
 *         other object, or <code>false</code> otherwise.
 */
template<typename T, typename U>
constexpr bool operator>(const optional_ref<T> &l, const U &r) { return l && r < *l; }

/**
 * Performs ">" comparison of an object and another object contained
 * in optional_ref.
 *
 * @param l object to compare
 * @param r optional_ref object to compare
 * @return <code>true</code> if l object is greater than the object contained in
 *         optional_ref parameter, or <code>false</code> otherwise.
 */
template<typename T, typename U>
constexpr bool operator>(const U &l, const optional_ref<T> &r) { return !r || *r < l; }

/**
 * Performs "<=" comparison of optional_ref object and other object
 * which can be compared to the object contained in optional_ref.
 *
 * @param l optional_ref object to compare
 * @param r object to compare
 * @return <code>true</code> if contained object of optional_ref less or equals
 *         to the other object, or <code>false</code> otherwise.
 */
template<typename T, typename U>
constexpr bool operator<=(const optional_ref<T> &l, const U &r) { return !l || !(r < *l); }

/**
 * Performs "<=" comparison of an object and another object contained
 * in optional_ref.
 *
 * @param l object to compare
 * @param r optional_ref object to compare
 * @return <code>true</code> if l object is less or equals to the object contained in
 *         optional_ref parameter, or <code>false</code> otherwise.
 */
template<typename T, typename U>
constexpr bool operator<=(const U &l, const optional_ref<T> &r) { return r && !(*r < l); }

/**
 * Performs ">=" comparison of optional_ref object and other object
 * which can be compared to the object contained in optional_ref.
 *
 * @param l optional_ref object to compare
 * @param r object to compare
 * @return <code>true</code> if contained object of optional_ref greater or equals
 *         to the other object, or <code>false</code> otherwise.
 */
template<typename T, typename U>
constexpr bool operator>=(const optional_ref<T> &l, const U &r) { return l && !(*l < r); }

/**
 * Performs ">=" comparison of an object and another object contained
 * in optional_ref.
 *
 * @param l object to compare
 * @param r optional_ref object to compare
 * @return <code>true</code> if l object is greater or equals to the object contained in
 *         optional_ref parameter, or <code>false</code> otherwise.
 */
template<typename T, typename U>
constexpr bool operator>=(const U &l, const optional_ref<T> &r) { return !r || !(l < *r); }

/**
 * Overloads the std::swap algorithm for optional_ref.
 *
 * Exchanges the state of l with that of r. Effectively calls l.swap(r)
 * @param l object to swap state with r
 * @param r object to swap state with l
 */
template<typename T>
inline void swap(optional_ref<T> &l, optional_ref<T> &r) noexcept(noexcept(l.swap(r))) { l.swap(r); }

} // end of servlet namespace

#endif // SERVLET_OPTIONAL_H
