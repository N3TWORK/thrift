namespace Thrift.Protocol 
{
	// interface for thrift typedef types ('typedef Foo Bar' => 'struct Bar : TTypedef<Foo>')
	public partial interface TTypedef<T> 
	{
		T Value { get; set; }
	}
}

