#pragma once

class Functor
{
public:
  virtual ~Functor() {}

  virtual void Invoke() = 0;
  virtual void operator()() = 0;
};

class FunctorF : public Functor
{
public:
  typedef void (*MethodType)();

  FunctorF(MethodType pFunction) : m_pFunction(pFunction) {}

  virtual void Invoke() { m_pFunction(); }
  virtual void operator()() { m_pFunction(); }

private:
  MethodType m_pFunction;
};

template<typename P1>
class FunctorFP1 : public Functor
{
public:
  typedef void (*MethodType)(P1);

  FunctorFP1(MethodType pFunction, const P1 p1) : m_pFunction(pFunction), m_p1(p1) {}

  virtual void Invoke() { m_pFunction(m_p1); }
  virtual void operator()() { m_pFunction(m_p1); }

private:
  MethodType m_pFunction;
  P1 m_p1;
};

template<typename P1, typename P2>
class FunctorFP2 : public Functor
{
public:
  typedef void (*MethodType)(P1, P2);

  FunctorFP2(MethodType pFunction, const P1 p1, const P2 p2) : m_pFunction(pFunction), m_p1(p1), m_p2(p2) {}

  virtual void Invoke() { m_pFunction(m_p1, m_p2); }
  virtual void operator()() { m_pFunction(m_p1, m_p2); }

private:
  MethodType m_pFunction;
  P1 m_p1;
  P2 m_p2;
};

template<typename P1, typename P2, typename P3>
class FunctorFP3 : public Functor
{
public:
  typedef void (*MethodType)(P1, P2, P3);

  FunctorFP3(MethodType pFunction, const P1 p1, const P2 p2, const P3 p3)
    : m_pFunction(pFunction), m_p1(p1), m_p2(p2), m_p3(p3)
  {
  }

  virtual void Invoke() { m_pFunction(m_p1, m_p2, m_p3); }
  virtual void operator()() { m_pFunction(m_p1, m_p2, m_p3); }

private:
  MethodType m_pFunction;
  P1 m_p1;
  P2 m_p2;
  P3 m_p3;
};

template<class Class>
class FunctorClass : public Functor
{
public:
  typedef void (Class::*MethodType)();

  FunctorClass(Class* pThis, MethodType pFunction) : m_pThis(pThis), m_pFunction(pFunction) {}
  FunctorClass(const Class* pThis, MethodType pFunction) : m_pThis(pThis), m_pFunction(pFunction) {}

  const Class* GetClassPointer() const { return m_pThis; }
  Class* GetClassPointer() { return m_pThis; }

  virtual void Invoke() { (m_pThis->*m_pFunction)(); }
  virtual void operator()() { (m_pThis->*m_pFunction)(); }

private:
  Class* m_pThis;
  MethodType m_pFunction;
};

template<class Class, typename P1>
class FunctorClassP1 : public Functor
{
public:
  typedef void (Class::*MethodType)(P1);

  FunctorClassP1(Class* pThis, MethodType pFunction, const P1 p1) : m_pThis(pThis), m_pFunction(pFunction), m_p1(p1) {}
  FunctorClassP1(const Class* pThis, MethodType pFunction, const P1 p1)
    : m_pThis(pThis), m_pFunction(pFunction), m_p1(p1)
  {
  }

  const Class* GetClassPointer() const { return m_pThis; }
  Class* GetClassPointer() { return m_pThis; }

  virtual void Invoke() { (m_pThis->*m_pFunction)(m_p1); }
  virtual void operator()() { (m_pThis->*m_pFunction)(m_p1); }

private:
  Class* m_pThis;
  MethodType m_pFunction;
  P1 m_p1;
};

template<class Class, typename P1, typename P2>
class FunctorClassP2 : public Functor
{
public:
  typedef void (Class::*MethodType)(P1, P2);

  FunctorClassP2(Class* pThis, MethodType pFunction, const P1 p1, const P2 p2)
    : m_pThis(pThis), m_pFunction(pFunction), m_p1(p1), m_p2(p2)
  {
  }
  FunctorClassP2(const Class* pThis, MethodType pFunction, const P1 p1, const P2 p2)
    : m_pThis(pThis), m_pFunction(pFunction), m_p1(p1), m_p2(p2)
  {
  }

  const Class* GetClassPointer() const { return m_pThis; }
  Class* GetClassPointer() { return m_pThis; }

  virtual void Invoke() { (m_pThis->*m_pFunction)(m_p1, m_p2); }
  virtual void operator()() { (m_pThis->*m_pFunction)(m_p1, m_p2); }

private:
  Class* m_pThis;
  MethodType m_pFunction;
  P1 m_p1;
  P2 m_p2;
};

template<class Class, typename P1, typename P2, typename P3>
class FunctorClassP3 : public Functor
{
public:
  typedef void (Class::*MethodType)(P1, P2, P3);

  FunctorClassP3(Class* pThis, MethodType pFunction, const P1 p1, const P2 p2, const P3 p3)
    : m_pThis(pThis), m_pFunction(pFunction), m_p1(p1), m_p2(p2), m_p3(p3)
  {
  }
  FunctorClassP3(const Class* pThis, MethodType pFunction, const P1 p1, const P2 p2, const P3 p3)
    : m_pThis(pThis), m_pFunction(pFunction), m_p1(p1), m_p2(p2), m_p3(p3)
  {
  }

  const Class* GetClassPointer() const { return m_pThis; }
  Class* GetClassPointer() { return m_pThis; }

  virtual void Invoke() { (m_pThis->*m_pFunction)(m_p1, m_p2, m_p3); }
  virtual void operator()() { (m_pThis->*m_pFunction)(m_p1, m_p2, m_p3); }

private:
  Class* m_pThis;
  MethodType m_pFunction;
  P1 m_p1;
  P2 m_p2;
  P3 m_p3;
};

template<typename RT>
class FunctorRT
{
public:
  FunctorRT() {}
  virtual ~FunctorRT() {}

  virtual RT Invoke() = 0;
  virtual RT operator()() = 0;
};

static inline FunctorF* MakeFunctor(FunctorF::MethodType pFunction)
{
  return new FunctorF(pFunction);
}
template<typename P1>
static inline FunctorFP1<P1>* MakeFunctor(typename FunctorFP1<P1>::MethodType pFunction, const P1 p1)
{
  return new FunctorFP1<P1>(pFunction, p1);
}
template<typename P1, typename P2>
static inline FunctorFP2<P1, P2>* MakeFunctor(typename FunctorFP2<P1, P2>::MethodType pFunction, const P1 p1,
                                              const P2 p2)
{
  return new FunctorFP2<P1, P2>(pFunction, p1, p2);
}
template<typename P1, typename P2, typename P3>
static inline FunctorFP3<P1, P2, P3>* MakeFunctor(typename FunctorFP3<P1, P2, P3>::MethodType pFunction, const P1 p1,
                                                  const P2 p2, const P3 p3)
{
  return new FunctorFP3<P1, P2, P3>(pFunction, p1, p2, p3);
}

template<class Class>
static inline FunctorClass<Class>* MakeFunctorClass(Class* pThis, typename FunctorClass<Class>::MethodType pFunction)
{
  return new FunctorClass<Class>(pThis, pFunction);
}
template<class Class, typename P1>
static inline FunctorClassP1<Class, P1>*
MakeFunctorClass(Class* pThis, typename FunctorClassP1<Class, P1>::MethodType pFunction, const P1 p1)
{
  return new FunctorClassP1<Class, P1>(pThis, pFunction, p1);
}
template<class Class, typename P1, typename P2>
static inline FunctorClassP2<Class, P1, P2>*
MakeFunctorClass(Class* pThis, typename FunctorClassP2<Class, P1, P2>::MethodType pFunction, const P1 p1, const P2 p2)
{
  return new FunctorClassP2<Class, P1, P2>(pThis, pFunction, p1, p2);
}
template<class Class, typename P1, typename P2, typename P3>
static inline FunctorClassP3<Class, P1, P2, P3>*
MakeFunctorClass(Class* pThis, typename FunctorClassP3<Class, P1, P2, P3>::MethodType pFunction, const P1 p1,
                 const P2 p2, const P3 p3)
{
  return new FunctorClassP3<Class, P1, P2, P3>(pThis, pFunction, p1, p2, p3);
}

template<class Class>
static inline FunctorClass<Class>* MakeFunctorClass(const Class* pThis,
                                                    typename FunctorClass<Class>::MethodType pFunction)
{
  return new FunctorClass<Class>(pThis, pFunction);
}
template<class Class, typename P1>
static inline FunctorClassP1<Class, P1>*
MakeFunctorClass(const Class* pThis, typename FunctorClassP1<Class, P1>::MethodType pFunction, const P1 p1)
{
  return new FunctorClassP1<Class, P1>(pThis, pFunction, p1);
}
template<class Class, typename P1, typename P2>
static inline FunctorClassP2<Class, P1, P2>*
MakeFunctorClass(const Class* pThis, typename FunctorClassP2<Class, P1, P2>::MethodType pFunction, const P1 p1,
                 const P2 p2)
{
  return new FunctorClassP2<Class, P1, P2>(pThis, pFunction, p1, p2);
}
template<class Class, typename P1, typename P2, typename P3>
static inline FunctorClassP3<Class, P1, P2, P3>*
MakeFunctorClass(const Class* pThis, typename FunctorClassP3<Class, P1, P2, P3>::MethodType pFunction, const P1 p1,
                 const P2 p2, const P3 p3)
{
  return new FunctorClassP3<Class, P1, P2, P3>(pThis, pFunction, p1, p2, p3);
}

template<typename A1>
class FunctorA1
{
public:
  virtual ~FunctorA1() {}

  virtual void Invoke(const A1 a1) = 0;
  virtual void operator()(const A1 a1) = 0;
};

template<typename A1, typename A2>
class FunctorA2
{
public:
  virtual ~FunctorA2() {}

  virtual void Invoke(const A1 a1, const A2 a2) = 0;
  virtual void operator()(const A1 a1, const A2 a2) = 0;
};

template<typename A1, typename A2, typename A3>
class FunctorA3
{
public:
  virtual ~FunctorA3() {}

  virtual void Invoke(const A1 a1, const A2 a2, const A3 a3) = 0;
  virtual void operator()(const A1 a1, const A2 a2, const A3 a3) = 0;
};

template<typename A1, typename A2, typename A3, typename A4>
class FunctorA4
{
public:
  virtual ~FunctorA4() {}

  virtual void Invoke(const A1 a1, const A2 a2, const A3 a3, const A4 a4) = 0;
  virtual void operator()(const A1 a1, const A2 a2, const A3 a3, const A4 a4) = 0;
};

template<typename A1, typename A2, typename A3, typename A4, typename A5>
class FunctorA5
{
public:
  virtual ~FunctorA5() {}

  virtual void Invoke(const A1 a1, const A2 a2, const A3 a3, const A4 a4, const A5 a5) = 0;
  virtual void operator()(const A1 a1, const A2 a2, const A3 a3, const A4 a4, const A5 a5) = 0;
};

template<typename A1, typename A2, typename A3, typename A4, typename A5, typename A6>
class FunctorA6
{
public:
  virtual ~FunctorA6() {}

  virtual void Invoke(const A1 a1, const A2 a2, const A3 a3, const A4 a4, const A5 a5, const A6 a6) = 0;
  virtual void operator()(const A1 a1, const A2 a2, const A3 a3, const A4 a4, const A5 a5, const A6 a6) = 0;
};

template<typename A1>
class FunctorFA1 : public FunctorA1<A1>
{
public:
  typedef void (*MethodType)(A1);

  FunctorFA1(MethodType pFunction) : m_pFunction(pFunction) {}

  virtual void Invoke(const A1 a1) { m_pFunction(a1); }
  virtual void operator()(const A1 a1) { m_pFunction(a1); }

private:
  MethodType m_pFunction;
};

template<typename A1, typename A2>
class FunctorFA2 : public FunctorA2<A1, A2>
{
public:
  typedef void (*MethodType)(A1, A2);

  FunctorFA2(MethodType pFunction) : m_pFunction(pFunction) {}

  virtual void Invoke(const A1 a1, const A2 a2) { m_pFunction(a1, a2); }
  virtual void operator()(const A1 a1, const A2 a2) { m_pFunction(a1, a2); }

private:
  MethodType m_pFunction;
};

template<typename A1, typename A2, typename A3>
class FunctorFA3 : public FunctorA3<A1, A2, A3>
{
public:
  typedef void (*MethodType)(A1, A2, A3);

  FunctorFA3(MethodType pFunction) : m_pFunction(pFunction) {}

  virtual void Invoke(const A1 a1, const A2 a2, const A3 a3) { m_pFunction(a1, a2, a3); }
  virtual void operator()(const A1 a1, const A2 a2, const A3 a3) { m_pFunction(a1, a2, a3); }

private:
  MethodType m_pFunction;
};

template<typename A1, typename A2, typename A3, typename A4>
class FunctorFA4 : public FunctorA4<A1, A2, A3, A4>
{
public:
  typedef void (*MethodType)(A1, A2, A3, A4);

  FunctorFA4(MethodType pFunction) : m_pFunction(pFunction) {}

  virtual void Invoke(const A1 a1, const A2 a2, const A3 a3, const A4 a4) { m_pFunction(a1, a2, a3, a4); }
  virtual void operator()(const A1 a1, const A2 a2, const A3 a3, const A4 a4) { m_pFunction(a1, a2, a3, a4); }

private:
  MethodType m_pFunction;
};

template<typename A1, typename A2, typename A3, typename A4, typename A5>
class FunctorFA5 : public FunctorA5<A1, A2, A3, A4, A5>
{
public:
  typedef void (*MethodType)(A1, A2, A3, A4, A5);

  FunctorFA5(MethodType pFunction) : m_pFunction(pFunction) {}

  virtual void Invoke(const A1 a1, const A2 a2, const A3 a3, const A4 a4, const A5 a5)
  {
    m_pFunction(a1, a2, a3, a4, a5);
  }
  virtual void operator()(const A1 a1, const A2 a2, const A3 a3, const A4 a4, const A5 a5)
  {
    m_pFunction(a1, a2, a3, a4, a5);
  }

private:
  MethodType m_pFunction;
};

template<typename A1, typename A2, typename A3, typename A4, typename A5, typename A6>
class FunctorFA6 : public FunctorA6<A1, A2, A3, A4, A5, A6>
{
public:
  typedef void (*MethodType)(A1, A2, A3, A4, A5, A6);

  FunctorFA6(MethodType pFunction) : m_pFunction(pFunction) {}

  virtual void Invoke(const A1 a1, const A2 a2, const A3 a3, const A4 a4, const A5 a5, const A6 a6)
  {
    m_pFunction(a1, a2, a3, a4, a5, a6);
  }
  virtual void operator()(const A1 a1, const A2 a2, const A3 a3, const A4 a4, const A5 a5, const A6 a6)
  {
    m_pFunction(a1, a2, a3, a4, a5, a6);
  }

private:
  MethodType m_pFunction;
};

template<class Class, typename A1>
class FunctorClassA1 : public FunctorA1<A1>
{
public:
  typedef void (Class::*MethodType)(A1);

  FunctorClassA1(Class* pThis, MethodType pFunction) : m_pThis(pThis), m_pFunction(pFunction) {}
  FunctorClassA1(const Class* pThis, MethodType pFunction) : m_pThis(pThis), m_pFunction(pFunction) {}

  const Class* GetClassPointer() const { return m_pThis; }
  Class* GetClassPointer() { return m_pThis; }

  virtual void Invoke(const A1 a1) { (m_pThis->*m_pFunction)(a1); }
  virtual void operator()(const A1 a1) { (m_pThis->*m_pFunction)(a1); }

private:
  Class* m_pThis;
  MethodType m_pFunction;
};

template<class Class, typename A1, typename A2>
class FunctorClassA2 : public FunctorA2<A1, A2>
{
public:
  typedef void (Class::*MethodType)(A1, A2);

  FunctorClassA2(Class* pThis, MethodType pFunction) : m_pThis(pThis), m_pFunction(pFunction) {}
  FunctorClassA2(const Class* pThis, MethodType pFunction) : m_pThis(pThis), m_pFunction(pFunction) {}

  const Class* GetClassPointer() const { return m_pThis; }
  Class* GetClassPointer() { return m_pThis; }

  virtual void Invoke(const A1 a1, const A2 a2) { (m_pThis->*m_pFunction)(a1, a2); }
  virtual void operator()(const A1 a1, const A2 a2) { (m_pThis->*m_pFunction)(a1, a2); }

private:
  Class* m_pThis;
  MethodType m_pFunction;
};

template<class Class, typename A1, typename A2, typename A3>
class FunctorClassA3 : public FunctorA3<A1, A2, A3>
{
public:
  typedef void (Class::*MethodType)(A1, A2, A3);

  FunctorClassA3(Class* pThis, MethodType pFunction) : m_pThis(pThis), m_pFunction(pFunction) {}
  FunctorClassA3(const Class* pThis, MethodType pFunction) : m_pThis(pThis), m_pFunction(pFunction) {}

  const Class* GetClassPointer() const { return m_pThis; }
  Class* GetClassPointer() { return m_pThis; }

  virtual void Invoke(const A1 a1, const A2 a2, const A3 a3) { (m_pThis->*m_pFunction)(a1, a2, a3); }
  virtual void operator()(const A1 a1, const A2 a2, const A3 a3) { (m_pThis->*m_pFunction)(a1, a2, a3); }

private:
  Class* m_pThis;
  MethodType m_pFunction;
};

template<class Class, typename A1, typename A2, typename A3, typename A4>
class FunctorClassA4 : public FunctorA4<A1, A2, A3, A4>
{
public:
  typedef void (Class::*MethodType)(A1, A2, A3, A4);

  FunctorClassA4(Class* pThis, MethodType pFunction) : m_pThis(pThis), m_pFunction(pFunction) {}
  FunctorClassA4(const Class* pThis, MethodType pFunction) : m_pThis(pThis), m_pFunction(pFunction) {}

  const Class* GetClassPointer() const { return m_pThis; }
  Class* GetClassPointer() { return m_pThis; }

  virtual void Invoke(const A1 a1, const A2 a2, const A3 a3, const A4 a4) { (m_pThis->*m_pFunction)(a1, a2, a3, a4); }
  virtual void operator()(const A1 a1, const A2 a2, const A3 a3, const A4 a4)
  {
    (m_pThis->*m_pFunction)(a1, a2, a3, a4);
  }

private:
  Class* m_pThis;
  MethodType m_pFunction;
};

template<class Class, typename A1, typename A2, typename A3, typename A4, typename A5>
class FunctorClassA5 : public FunctorA5<A1, A2, A3, A4, A5>
{
public:
  typedef void (Class::*MethodType)(A1, A2, A3, A4, A5);

  FunctorClassA5(Class* pThis, MethodType pFunction) : m_pThis(pThis), m_pFunction(pFunction) {}
  FunctorClassA5(const Class* pThis, MethodType pFunction) : m_pThis(pThis), m_pFunction(pFunction) {}

  const Class* GetClassPointer() const { return m_pThis; }
  Class* GetClassPointer() { return m_pThis; }

  virtual void Invoke(const A1 a1, const A2 a2, const A3 a3, const A4 a4, const A5 a5)
  {
    (m_pThis->*m_pFunction)(a1, a2, a3, a4, a5);
  }
  virtual void operator()(const A1 a1, const A2 a2, const A3 a3, const A4 a4, const A5 a5)
  {
    (m_pThis->*m_pFunction)(a1, a2, a3, a4, a5);
  }

private:
  Class* m_pThis;
  MethodType m_pFunction;
};

template<class Class, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6>
class FunctorClassA6 : public FunctorA6<A1, A2, A3, A4, A5, A6>
{
public:
  typedef void (Class::*MethodType)(A1, A2, A3, A4, A5, A6);

  FunctorClassA6(Class* pThis, MethodType pFunction) : m_pThis(pThis), m_pFunction(pFunction) {}
  FunctorClassA6(const Class* pThis, MethodType pFunction) : m_pThis(pThis), m_pFunction(pFunction) {}

  const Class* GetClassPointer() const { return m_pThis; }
  Class* GetClassPointer() { return m_pThis; }

  virtual void Invoke(const A1 a1, const A2 a2, const A3 a3, const A4 a4, const A5 a5, const A6 a6)
  {
    (m_pThis->*m_pFunction)(a1, a2, a3, a4, a5, a6);
  }
  virtual void operator()(const A1 a1, const A2 a2, const A3 a3, const A4 a4, const A5 a5, const A6 a6)
  {
    (m_pThis->*m_pFunction)(a1, a2, a3, a4, a5, a6);
  }

private:
  Class* m_pThis;
  MethodType m_pFunction;
};

template<typename A1>
static inline FunctorFA1<A1>* MakeArgFunctor(typename FunctorFA1<A1>::MethodType pFunction)
{
  return new FunctorFA1<A1>(pFunction);
}
template<typename A1, typename A2>
static inline FunctorFA2<A1, A2>* MakeArgFunctor(typename FunctorFA2<A1, A2>::MethodType pFunction)
{
  return new FunctorFA2<A1, A2>(pFunction);
}
template<typename A1, typename A2, typename A3>
static inline FunctorFA3<A1, A2, A3>* MakeArgFunctor(typename FunctorFA3<A1, A2, A3>::MethodType pFunction)
{
  return new FunctorFA3<A1, A2, A3>(pFunction);
}
template<typename A1, typename A2, typename A3, typename A4>
static inline FunctorFA4<A1, A2, A3, A4>* MakeArgFunctor(typename FunctorFA4<A1, A2, A3, A4>::MethodType pFunction)
{
  return new FunctorFA4<A1, A2, A3, A4>(pFunction);
}
template<typename A1, typename A2, typename A3, typename A4, typename A5>
static inline FunctorFA5<A1, A2, A3, A4, A5>*
MakeArgFunctor(typename FunctorFA5<A1, A2, A3, A4, A5>::MethodType pFunction)
{
  return new FunctorFA5<A1, A2, A3, A4, A5>(pFunction);
}
template<typename A1, typename A2, typename A3, typename A4, typename A5, typename A6>
static inline FunctorFA6<A1, A2, A3, A4, A5, A6>*
MakeArgFunctor(typename FunctorFA6<A1, A2, A3, A4, A5, A6>::MethodType pFunction)
{
  return new FunctorFA6<A1, A2, A3, A4, A5, A6>(pFunction);
}

template<typename A1, class Class>
static inline FunctorClassA1<Class, A1>* MakeArgFunctor(Class* pThis,
                                                        typename FunctorClassA1<Class, A1>::MethodType pFunction)
{
  return new FunctorClassA1<Class, A1>(pThis, pFunction);
}
template<typename A1, typename A2, class Class>
static inline FunctorClassA2<Class, A1, A2>*
MakeArgFunctor(Class* pThis, typename FunctorClassA2<Class, A1, A2>::MethodType pFunction)
{
  return new FunctorClassA2<Class, A1, A2>(pThis, pFunction);
}
template<typename A1, typename A2, typename A3, class Class>
static inline FunctorClassA3<Class, A1, A2, A3>*
MakeArgFunctor(Class* pThis, typename FunctorClassA3<Class, A1, A2, A3>::MethodType pFunction)
{
  return new FunctorClassA3<Class, A1, A2, A3>(pThis, pFunction);
}
template<typename A1, typename A2, typename A3, typename A4, class Class>
static inline FunctorClassA4<Class, A1, A2, A3, A4>*
MakeArgFunctor(Class* pThis, typename FunctorClassA4<Class, A1, A2, A3, A4>::MethodType pFunction)
{
  return new FunctorClassA4<Class, A1, A2, A3, A4>(pThis, pFunction);
}
template<typename A1, typename A2, typename A3, typename A4, typename A5, class Class>
static inline FunctorClassA5<Class, A1, A2, A3, A4, A5>*
MakeArgFunctor(Class* pThis, typename FunctorClassA5<Class, A1, A2, A3, A4, A5>::MethodType pFunction)
{
  return new FunctorClassA5<Class, A1, A2, A3, A4, A5>(pThis, pFunction);
}
template<typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, class Class>
static inline FunctorClassA6<Class, A1, A2, A3, A4, A5, A6>*
MakeArgFunctor(Class* pThis, typename FunctorClassA6<Class, A1, A2, A3, A4, A5, A6>::MethodType pFunction)
{
  return new FunctorClassA6<Class, A1, A2, A3, A4, A5, A6>(pThis, pFunction);
}

template<typename A1, class Class>
static inline FunctorClassA1<Class, A1>* MakeArgFunctor(const Class* pThis,
                                                        typename FunctorClassA1<Class, A1>::MethodType pFunction)
{
  return new FunctorClassA1<Class, A1>(pThis, pFunction);
}
template<typename A1, typename A2, class Class>
static inline FunctorClassA2<Class, A1, A2>*
MakeArgFunctor(const Class* pThis, typename FunctorClassA2<Class, A1, A2>::MethodType pFunction)
{
  return new FunctorClassA2<Class, A1, A2>(pThis, pFunction);
}
template<typename A1, typename A2, typename A3, class Class>
static inline FunctorClassA3<Class, A1, A2, A3>*
MakeArgFunctor(const Class* pThis, typename FunctorClassA3<Class, A1, A2, A3>::MethodType pFunction)
{
  return new FunctorClassA3<Class, A1, A2, A3>(pThis, pFunction);
}
template<typename A1, typename A2, typename A3, typename A4, class Class>
static inline FunctorClassA4<Class, A1, A2, A3, A4>*
MakeArgFunctor(const Class* pThis, typename FunctorClassA4<Class, A1, A2, A3, A4>::MethodType pFunction)
{
  return new FunctorClassA4<Class, A1, A2, A3, A4>(pThis, pFunction);
}
template<typename A1, typename A2, typename A3, typename A4, typename A5, class Class>
static inline FunctorClassA5<Class, A1, A2, A3, A4, A5>*
MakeArgFunctor(const Class* pThis, typename FunctorClassA5<Class, A1, A2, A3, A4, A5>::MethodType pFunction)
{
  return new FunctorClassA5<Class, A1, A2, A3, A4, A5>(pThis, pFunction);
}
template<typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, class Class>
static inline FunctorClassA6<Class, A1, A2, A3, A4, A5, A6>*
MakeArgFunctor(const Class* pThis, typename FunctorClassA6<Class, A1, A2, A3, A4, A5, A6>::MethodType pFunction)
{
  return new FunctorClassA6<Class, A1, A2, A3, A4, A5, A6>(pThis, pFunction);
}
