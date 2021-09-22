#include <QTest>

#include "trace_viewer/trace_tools.h"
#include "trace_viewer/filter_model.h"

#include <trace_x/trace_x.h>

class __my_type
{

};

void function_1()
{
    qDebug() << ___TRACE_CURRENT_FUNCTION_;
}

__my_type function_2()
{
    qDebug() << ___TRACE_CURRENT_FUNCTION_;

    return __my_type();
}

std::pair<int, float> function_3(double = 0)
{
    qDebug() << ___TRACE_CURRENT_FUNCTION_;

    return std::pair<int, float>();
}

std::pair<int, std::pair<int, float> > function_4(double = 0)
{
    qDebug() << ___TRACE_CURRENT_FUNCTION_;

    return std::pair<int, std::pair<int, float> >();
}

namespace space_1
{
namespace space_2
{
void function()
{
    qDebug() << ___TRACE_CURRENT_FUNCTION_;
}

template<class T>
class MyClass
{
public:
    MyClass()
    {
        qDebug() << ___TRACE_CURRENT_FUNCTION_;
    }

    template<class U>
    class InsClass
    {
    public:
        void function()
        {
            qDebug() << ___TRACE_CURRENT_FUNCTION_;
        }

        template<class K>
        std::pair<int, std::pair<int, float> > function_1(double = 0, const std::pair< K, std::pair<int, float> > & = std::pair< K, std::pair<int, float> >()) const
        {
            qDebug() << ___TRACE_CURRENT_FUNCTION_;

            return std::pair<int, std::pair<int, float> >();
        }

        class InsClass & operator ()(int t)
        {
            qDebug() << ___TRACE_CURRENT_FUNCTION_ ;

            return *this;
        }
    };

    template<class K>
    void foo1 (K, std::vector<K> = std::vector<K>()) const
    {
        InsClass<char> dd;

        dd.function();

        qDebug() << ___TRACE_CURRENT_FUNCTION_;
    }

    virtual void foo2(std::vector<float> *) const
    {
        qDebug() << ___TRACE_CURRENT_FUNCTION_;
    }
};

}
}

//

class Test : public QObject
{
    Q_OBJECT

private slots:

    void test_filter_1()
    {
        //Включаем процессы "process 1", "process 2" с модулями "module 1", "module 2"

        /*
         + process name
           p1
           p2
           + module name
             m1
             m2
         */
        FilterModel filter =
                FilterModel() << (FilterGroup(IncludeOperator, ProcessNameEntity, QList<FilterItem>() << "process 1" << "process 2") <<
                                  FilterGroup(IncludeOperator, ModuleNameEntity, QList<FilterItem>() << "module 1" << "module 2"));

        QVERIFY(filter.check_filter(TraceMessageDescription
                                    (ProcessNameEntity, "process 1")
                                    (ModuleNameEntity, "module 2")));

        QVERIFY(!filter.check_filter(TraceMessageDescription
                                     (ProcessNameEntity, "process 1")
                                     (ModuleNameEntity, "module 3")));
    }

    void test_filter_2()
    {
        //Включаем процессы "process 1", "process 2" с модулями "module 1", "module 2", а также всё от "process 3"

        /*
             + process name
               p1
               p2
               + module name
                 m1
                 m2
             + process name
               p3
             */

        FilterModel filter =
                FilterModel() << (FilterGroup(IncludeOperator, ProcessNameEntity, QList<FilterItem>() << "process 1" << "process 2") <<
                                  FilterGroup(IncludeOperator, ModuleNameEntity, QList<FilterItem>() << "module 1" << "module 2")) <<

                                 FilterGroup(IncludeOperator, ProcessNameEntity, QList<FilterItem>() << "process 3");

        //

        QVERIFY(filter.check_filter(TraceMessageDescription
                                    (ProcessNameEntity, "process 1")
                                    (ModuleNameEntity, "module 2")));

        //TODO убрать включение в первую группу по-умолчанию

        QVERIFY(filter.check_filter(TraceMessageDescription
                                    (ProcessNameEntity, "process 3")
                                    (ModuleNameEntity, "module 3")));

        QVERIFY(!filter.check_filter(TraceMessageDescription
                                     (ProcessNameEntity, "process 4")
                                     (ModuleNameEntity, "module 3")));

        QVERIFY(!filter.check_filter(TraceMessageDescription
                                     (ProcessNameEntity, "process 2")
                                     (ModuleNameEntity, "module 3")));
    }

    void test_filter_3()
    {
        //Включаем только "process 1" и "process 2";

        /*
             + process name
               p1
               p2
             */

        FilterModel filter = FilterModel() << FilterGroup(IncludeOperator, ProcessNameEntity, QList<FilterItem>() << "process 1" << "process 2");

        QVERIFY(filter.check_filter(TraceMessageDescription
                                    (ProcessNameEntity, "process 1")
                                    (ModuleNameEntity, "module 2")));

        QVERIFY(!filter.check_filter(TraceMessageDescription
                                     (ProcessNameEntity, "process 3")
                                     (ModuleNameEntity, "module 2")));
    }

    void test_filter_4()
    {
        //Включить "class 1", "function 1". Исключить "source 1".

        /*
             + class name
               class 1
             + function name
               function 1
             - source name
               source 1
             */

        // class 1 + function 1 + !source1

        FilterModel filter;

        filter << FilterGroup(IncludeOperator, ClassNameEntity, QList<FilterItem>() << "class 1");
        filter << FilterGroup(IncludeOperator, FunctionNameEntity, QList<FilterItem>() << "function 1");
        filter << FilterGroup(ExcludeOperator, SourceNameEntity, QList<FilterItem>() << "source 1");

        QVERIFY(filter.check_filter(TraceMessageDescription
                                    (ClassNameEntity, "class 1")
                                    (FunctionNameEntity, "function 3")
                                    (SourceNameEntity, "source 2")
                                    ));

        QVERIFY(filter.check_filter(TraceMessageDescription
                                    (ClassNameEntity, "class 1")
                                    (FunctionNameEntity, "function 1")
                                    (SourceNameEntity, "source 2")
                                    ));

        QVERIFY(!filter.check_filter(TraceMessageDescription
                                     (ProcessNameEntity, "process 1")
                                     (FunctionNameEntity, "function 3")
                                     (SourceNameEntity, "source 3")
                                     ));

        QVERIFY(!filter.check_filter(TraceMessageDescription
                                     (ClassNameEntity, "class 1")
                                     (FunctionNameEntity, "function 3")
                                     (SourceNameEntity, "source 1")
                                     ));
    }

    void test_filter_5()
    {
        //Включить классы c1, c2 с модулями, отличными от m1, m2

        /*
             + class name
               c1
               c2
               - module name
                 m1
                 m2
             */

        // (c1; c2) & !(m1; m2)

        FilterModel filter =
                FilterModel() << (FilterGroup(IncludeOperator, ClassNameEntity, QList<FilterItem>() << "c1"  << "c2") <<
                                  FilterGroup(ExcludeOperator, ModuleNameEntity, QList<FilterItem>() << "m1"  << "m2"));

        QVERIFY(filter.check_filter(TraceMessageDescription
                                    (ModuleNameEntity, "m3")
                                    (ClassNameEntity, "c2")
                                    ));

        QVERIFY(!filter.check_filter(TraceMessageDescription
                                     (ModuleNameEntity, "m1")
                                     (ClassNameEntity, "c3")
                                     ));

        QVERIFY(!filter.check_filter(TraceMessageDescription
                                     (ModuleNameEntity, "m1")
                                     (ClassNameEntity, "c1")
                                     ));
    }

    void test_filter_6()
    {
        //Исключить m1, m2 с классами с1, с2

        /*
             - module name
               m1
               m2
               + class name
                 c1
                 c2
             */

        // !(m1; m2) & (c1; c2)

        FilterModel filter =
                FilterModel() << (FilterGroup(ExcludeOperator, ModuleNameEntity, QList<FilterItem>() << "m1"  << "m2") <<
                                  FilterGroup(IncludeOperator, ClassNameEntity, QList<FilterItem>() << "c1"  << "c2"));

        QVERIFY(filter.check_filter(TraceMessageDescription
                                    (ModuleNameEntity, "m1")
                                    (ClassNameEntity, "c3")
                                    ));

        QVERIFY(filter.check_filter(TraceMessageDescription
                                    (ModuleNameEntity, "m3")
                                    (ClassNameEntity, "c1")
                                    ));

        QVERIFY(!filter.check_filter(TraceMessageDescription
                                     (ModuleNameEntity, "m1")
                                     (ClassNameEntity, "c2")
                                     ));
    }

    void test_filter_7()
    {
        //Исключить DATA

        /*
             - message type
               DATA
            */

        FilterModel filter;

        filter << FilterGroup(ExcludeOperator, MessageTypeEntity, QList<FilterItem>() << "DATA");

        QVERIFY(filter.check_filter(TraceMessageDescription
                                    (ModuleNameEntity, "m1")
                                    (MessageTypeEntity, "INFO")
                                    ));

        QVERIFY(!filter.check_filter(TraceMessageDescription
                                     (ModuleNameEntity, "m3")
                                     (MessageTypeEntity, "DATA")
                                     ));
    }

    void test_filter_8()
    {
        //Включить только p1. Из этих сообщений исключить f1, f2, DATA

        /*
                 + process name
                   p1
                   - function name
                     f1
                     f2
                   - message
                     DATA
                 */

        // p1 & (!(f1 ; f2) + !DATA)

        FilterModel filter =
                FilterModel() << (FilterGroup(IncludeOperator, ProcessNameEntity, QList<FilterItem>() << "p1") <<
                                  FilterGroup(ExcludeOperator, FunctionNameEntity, QList<FilterItem>() << "f1"  << "f2") <<
                                  FilterGroup(ExcludeOperator, MessageTypeEntity, QList<FilterItem>() << "DATA"));

        QVERIFY(filter.check_filter(TraceMessageDescription
                                    (ProcessNameEntity, "p1")
                                    (FunctionNameEntity, "f3")
                                    (MessageTypeEntity, "ERROR")
                                    ));

        QVERIFY(!filter.check_filter(TraceMessageDescription
                                     (ProcessNameEntity, "p2")
                                     (FunctionNameEntity, "f1")
                                     (MessageTypeEntity, "INFO")
                                     ));

        QVERIFY(!filter.check_filter(TraceMessageDescription
                                     (ProcessNameEntity, "p1")
                                     (FunctionNameEntity, "f3")
                                     (MessageTypeEntity, "DATA")
                                     ));

        QVERIFY(!filter.check_filter(TraceMessageDescription
                                     (ProcessNameEntity, "p1")
                                     (FunctionNameEntity, "f1")
                                     (MessageTypeEntity, "ERROR")
                                     ));
    }

    void test_filter_9()
    {
        //Включить только p1. Из этих сообщений исключить f1 & INFO

        /*
                 + process name
                   p1
                   - function name
                     f1
                     + message
                       INFO
                 */

        // p1 & !f1 & INFO

        FilterModel filter =
                FilterModel() << (FilterGroup(IncludeOperator, ProcessNameEntity, QList<FilterItem>() << "p1") <<
                                  (FilterGroup(ExcludeOperator, FunctionNameEntity, QList<FilterItem>() << "f1") <<
                                   FilterGroup(IncludeOperator, MessageTypeEntity, QList<FilterItem>() << "INFO")));

        QVERIFY(filter.check_filter(TraceMessageDescription
                                    (ProcessNameEntity, "p1")
                                    (FunctionNameEntity, "f2")
                                    (MessageTypeEntity, "INFO")
                                    ));

        QVERIFY(filter.check_filter(TraceMessageDescription
                                    (ProcessNameEntity, "p1")
                                    (FunctionNameEntity, "f1")
                                    (MessageTypeEntity, "ERROR")
                                    ));

        QVERIFY(!filter.check_filter(TraceMessageDescription
                                     (ProcessNameEntity, "p2")
                                     (FunctionNameEntity, "f2")
                                     (MessageTypeEntity, "ERROR")
                                     ));

        QVERIFY(!filter.check_filter(TraceMessageDescription
                                     (ProcessNameEntity, "p1")
                                     (FunctionNameEntity, "f1")
                                     (MessageTypeEntity, "INFO")
                                     ));
    }

    void test_filter_10()
    {
        //Включить только p1 & f1, p1 & ERROR

        /*
             + process name
               p1
               + function name
                 f1
               + message
                 ERROR
            */

        //p1 & (f1 + ERROR)

        FilterModel filter =
                FilterModel() << (FilterGroup(IncludeOperator, ProcessNameEntity, QList<FilterItem>() << "p1") <<
                                  FilterGroup(IncludeOperator, FunctionNameEntity, QList<FilterItem>() << "f1") <<
                                  FilterGroup(IncludeOperator, MessageTypeEntity, QList<FilterItem>() << "ERROR"));

        QVERIFY(filter.check_filter(TraceMessageDescription
                                    (ProcessNameEntity, "p1")
                                    (FunctionNameEntity, "f1")
                                    (MessageTypeEntity, "INFO")
                                    ));

        QVERIFY(filter.check_filter(TraceMessageDescription
                                    (ProcessNameEntity, "p1")
                                    (FunctionNameEntity, "f1")
                                    (MessageTypeEntity, "ERROR")
                                    ));

        QVERIFY(filter.check_filter(TraceMessageDescription
                                    (ProcessNameEntity, "p1")
                                    (FunctionNameEntity, "f1")
                                    (MessageTypeEntity, "INFO")
                                    ));

        QVERIFY(!filter.check_filter(TraceMessageDescription
                                     (ProcessNameEntity, "p2")
                                     (FunctionNameEntity, "f1")
                                     (MessageTypeEntity, "ERROR")
                                     ));

        QVERIFY(!filter.check_filter(TraceMessageDescription
                                     (ProcessNameEntity, "p2")
                                     (FunctionNameEntity, "f2")
                                     (MessageTypeEntity, "DATA")
                                     ));
    }

    void test_print()
    {
        function_1();
        function_2();
        function_3();
        function_4();

        space_1::space_2::function();

        space_1::space_2::MyClass< std::pair<float, float> > cc;

        cc.foo1<float>(8.0);

        std::vector<float> p;

        cc.foo2(&p);
    }

    void test_parse_function_1()
    {
        const char f[] = "struct std::pair<unsigned int,unsigned short> __thiscall TraceController::register_function(const struct function_t &,unsigned short,bool)";

        function_t result = parse_function(f, sizeof(f) - 1);

        QCOMPARE(result.full_name, QString("TraceController::register_function"));
        QCOMPARE(result.spec, QString("__thiscall"));
    }

    void test_parse_function_2()
    {
        const char f[] = "std::pair<unsigned int,unsigned short> __thiscall TraceController::register_function(const struct function_t &,unsigned short,bool)";

        function_t result = parse_function(f, sizeof(f) - 1);

        QCOMPARE(result.full_name, QString("TraceController::register_function"));
        QCOMPARE(result.spec, QString("__thiscall"));
    }

    void test_parse_function_3()
    {
        const char f[] = "void TraceController::register_function(int)";

        function_t result = parse_function(f, sizeof(f) - 1);

        QCOMPARE(result.full_name, QString("TraceController::register_function"));
        QCOMPARE(result.spec, QString());
    }

    void test_parse_function_4()
    {
        const char f[] = "void __thiscall TraceController::register_function(int)";

        function_t result = parse_function(f, sizeof(f) - 1);

        QCOMPARE(result.full_name, QString("TraceController::register_function"));
        QCOMPARE(result.spec, QString("__thiscall"));
    }

    void test_parse_function_5()
    {
        const char f[] = "struct std::pair<int,struct std::pair<int,float> > __thiscall space_1::space_2::MyClass<struct std::pair<float,float> >::InsClass<char>::function_1<float>(double,const struct std::pair<float,struct std::pair<int,float> > &) const";

        function_t result = parse_function(f, sizeof(f) - 1);

        QCOMPARE(result.full_name, QString("space_1::space_2::MyClass::InsClass::function_1"));
        QCOMPARE(result.spec, QString("__thiscall"));
    }

    void test_parse_function_6()
    {
        const char f[] = "void __cdecl function_1(void)";

        function_t result = parse_function(f, sizeof(f) - 1);

        QCOMPARE(result.full_name, QString("function_1"));
        QCOMPARE(result.spec, QString("__cdecl"));
    }

    void test_parse_function_7()
    {
        const char f[] = "struct std::pair<int,float> __cdecl function_3(double)";

        function_t result = parse_function(f, sizeof(f) - 1);

        QCOMPARE(result.full_name, QString("function_3"));
        QCOMPARE(result.spec, QString("__cdecl"));
    }

    void test_parse_function_8()
    {
        const char f[] = "struct std::pair<int,struct std::pair<int,float> > __cdecl function_4(double)";

        function_t result = parse_function(f, sizeof(f) - 1);

        QCOMPARE(result.full_name, QString("function_4"));
        QCOMPARE(result.spec, QString("__cdecl"));
    }

    void test_parse_function_9()
    {
        const char f[] = "__thiscall space_1::space_2::MyClass<struct std::pair<float,float> >::MyClass(void)";

        function_t result = parse_function(f, sizeof(f) - 1);

        QCOMPARE(result.full_name, QString("space_1::space_2::MyClass::MyClass"));
        QCOMPARE(result.spec, QString("__thiscall"));
    }

    void test_parse_function_10()
    {
        const char f[] = "void __thiscall space_1::space_2::MyClass<struct std::pair<float,float> >::InsClass<char>::function(void)";

        function_t result = parse_function(f, sizeof(f) - 1);

        QCOMPARE(result.full_name, QString("space_1::space_2::MyClass::InsClass::function"));
        QCOMPARE(result.spec, QString("__thiscall"));
    }

    void test_parse_function_11()
    {
        const char f[] = "void __thiscall space_1::space_2::MyClass<struct std::pair<float,float> >::foo1<float>(float,class std::vector<float,class std::allocator<float> >) const";

        function_t result = parse_function(f, sizeof(f) - 1);

        QCOMPARE(result.full_name, QString("space_1::space_2::MyClass::foo1"));
        QCOMPARE(result.spec, QString("__thiscall"));
    }

    void test_parse_function_12()
    {
        const char f[] = "void __thiscall space_1::space_2::MyClass<struct std::pair<float,float> >::foo2(class std::vector<float,class std::allocator<float> > *) const";

        function_t result = parse_function(f, sizeof(f) - 1);

        QCOMPARE(result.full_name, QString("space_1::space_2::MyClass::foo2"));
        QCOMPARE(result.spec, QString("__thiscall"));
    }

    void test_parse_function_13()
    {
        const char f[] = "void function_1()";

        function_t result = parse_function(f, sizeof(f) - 1);

        QCOMPARE(result.full_name, QString("function_1"));
        QCOMPARE(result.return_type, QString("void"));
        QCOMPARE(result.spec, QString());
    }

    void test_parse_function_14()
    {
        const char f[] = "space_1::space_2::MyClass<T>::MyClass() [with T = std::pair<float, float>]";

        function_t result = parse_function(f, sizeof(f) - 1);

        QCOMPARE(result.full_name, QString("space_1::space_2::MyClass::MyClass"));
        QCOMPARE(result.return_type, QString());
        QCOMPARE(result.spec, QString());
    }

    void test_parse_function_15()
    {
        const char f[] = "void space_1::space_2::MyClass<T>::InsClass<U>::function() [with U = char; T = std::pair<float, float>]";

        function_t result = parse_function(f, sizeof(f) - 1);

        QCOMPARE(result.full_name, QString("space_1::space_2::MyClass::InsClass::function"));
        QCOMPARE(result.return_type, QString("void"));
        QCOMPARE(result.spec, QString());
    }

    void test_parse_function_16()
    {
        const char f[] = "void space_1::space_2::MyClass<T>::foo1(K, std::vector<K>) const [with K = float; T = std::pair<float, float>]";

        function_t result = parse_function(f, sizeof(f) - 1);

        QCOMPARE(result.full_name, QString("space_1::space_2::MyClass::foo1"));
        QCOMPARE(result.return_type, QString("void"));
        QCOMPARE(result.spec, QString());
    }

    void test_parse_function_17()
    {
        const char f[] = "class FilterListModel &__thiscall FilterListModel::operator =(const class FilterListModel &)";

        function_t result = parse_function(f, sizeof(f) - 1);

        QCOMPARE(result.full_name, QString("FilterListModel::operator ="));
        QCOMPARE(result.return_type, QString("FilterListModel"));
        QCOMPARE(result.spec, QString("__thiscall"));
    }

    void test_parse_function_18()
    {
        const char f[] = "class space_1::space_2::MyClass<int>::InsClass<float> &__thiscall space_1::space_2::MyClass<int>::InsClass<float>::operator ()(int)";

        function_t result = parse_function(f, sizeof(f) - 1);

        QCOMPARE(result.full_name, QString("space_1::space_2::MyClass::InsClass::operator ()"));
        QCOMPARE(result.return_type, QString("space_1::space_2::MyClass<int>::InsClass<float>"));
        QCOMPARE(result.spec, QString("__thiscall"));
    }

    void test_parse_function_19()
    {
        const char f[] = "class FilterGroup &__thiscall FilterGroup::operator <<(const class FilterItem &)";

        function_t result = parse_function(f, sizeof(f) - 1);

        QCOMPARE(result.full_name, QString("FilterGroup::operator <<"));
        QCOMPARE(result.return_type, QString("FilterGroup"));
        QCOMPARE(result.spec, QString("__thiscall"));
    }

    void test_parse_function_20()
    {
        const char f[] = "void __thiscall space_1::`anonymous-namespace'::<lambda6>::operator ()(void) const";

        function_t result = parse_function(f, sizeof(f) - 1);

        QCOMPARE(result.full_name, QString("space_1::`anonymous-namespace'::<lambda6>::operator ()"));
        QCOMPARE(result.return_type, QString("void"));
        QCOMPARE(result.spec, QString("__thiscall"));
    }

    void test_parse_function_21()
    {
        const char f[] = "lib3_test";

        function_t result = parse_function(f, sizeof(f) - 1);

        QCOMPARE(result.full_name, QString("lib3_test"));
        QCOMPARE(result.return_type, QString(""));
        QCOMPARE(result.spec, QString(""));
    }
};

QTEST_MAIN(Test)

#include "test_parse_funcsig.moc"
