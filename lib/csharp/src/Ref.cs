namespace Thrift
{
	// Ref is used to hold a nullable-reference to a value type (i.e., for optional struct values)
    public class Ref<T> where T : struct
    {
        public T Value;
    }
}

