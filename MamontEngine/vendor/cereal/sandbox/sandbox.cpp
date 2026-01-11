/*
  Copyright (c) 2014, Randolph Voorhies, Shane Grant
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:
      * Redistributions of source code must retain the above copyright
        notice, this list of conditions and the following disclaimer.
      * Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.
      * Neither the name of the copyright holder nor the
        names of its contributors may be used to endorse or promote products
        derived from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/



//namespace cereal
//{
//  template <>
//  struct LoadAndConstruct<NoDefaultCtor>
//  {
//    template <class Archive>
//    static void load_and_construct( Archive & ar, cereal::construct<NoDefaultCtor> & construct )
//    {
//      int y;
//      ar( y );
//      construct( y );
//    }
//  };
//}



// ######################################################################
int main()
{
  //std::cout << std::boolalpha << std::endl;

  /*Everything e_out;
  e_out.x = 99;
  e_out.y = 100;
  e_out.t1 = {1};
  e_out.t2 = {2};
  e_out.t3 = {3};
  e_out.t4 = {4};
  e_out.s = "Hello, World!";
  std::unique_ptr<NoDefaultCtor> nodefault( new NoDefaultCtor( 3 ) );

  Test2 t2 = {22};

  {
    std::ofstream os("out.txt", std::ios::binary);
    cereal::BinaryOutputArchive archive(os);
    archive(CEREAL_NVP(e_out));
    archive(t2);
    archive(nodefault);
  }

  Everything e_in;

  std::unique_ptr<NoDefaultCtor> nodefaultin( new NoDefaultCtor( 1 ) );

  {
    std::ifstream is("out.txt", std::ios::binary);
    cereal::BinaryInputArchive archive(is);
    archive(CEREAL_NVP(e_in));
    archive(t2);
    archive(nodefaultin);
    std::remove("out.txt");
  }

//  assert(e_in == e_out);
  assert(nodefault->y == nodefaultin->y);

  {
    cereal::BinaryOutputArchive archive(std::cout);
    int xxx[] = {-1, 95, 3};
    archive( xxx );

    cereal::XMLOutputArchive archive2(std::cout, cereal::XMLOutputArchive::Options(std::numeric_limits<double>::max_digits10, true, true));
    archive2( xxx );

    std::vector<int> yyy = {1, 2, 3};
    archive2( yyy );

    archive2.saveBinaryValue( xxx, sizeof(int)*3 );
  }

  {
    std::ofstream os("out.xml");
    cereal::XMLOutputArchive oar( os );
    //cereal::XMLOutputArchive oar( std::cout );

    oar( cereal::make_nvp("hello", 5 ) );

    std::string bla("bla");
    oar( bla );

    auto intptr = std::make_shared<int>(99);
    oar( CEREAL_NVP(intptr) );

    std::map<std::string, int> map1 =
    {
      {"one",   1},
      {"two",   2},
      {"three", 3}
    };

    oar( CEREAL_NVP(map1) );

    int x = 3;
    oar( CEREAL_NVP(x) );
    oar( 5 );
    oar( 3.3 );
    oar( 3.2f );
    oar( true );

    std::array<int,5> arr = {{1, 2, 3, 4, 5}};
    oar( arr );

    std::vector<std::string> vec = {"hey",
                                    "there",
                                    "buddy"};

    std::vector<std::vector<std::string>> vec2 = {vec, vec, vec};

    oar( cereal::make_nvp("EVERYTHING", e_out) );
    oar( vec );
    oar( vec2 );

    int xxx[] = {-1, 95, 3};
    oar.saveBinaryValue( xxx, sizeof(int)*3, "xxxbinary" );
    //oar.saveBinaryValue( xxx, sizeof(int)*3 );

    std::unique_ptr<Derived> d1( new Derived(3, 4) );
    std::unique_ptr<Base> d2( new Derived(4, 5) );
    std::shared_ptr<Base> d3( new Derived(5, 6) );
    oar( d1 );
    oar( d2 );
    oar( d3 );
  }

  {
    std::ifstream is("out.xml");
    cereal::XMLInputArchive iar( is );

    int hello;
    iar( cereal::make_nvp("hello", hello) );
    assert( hello == 5 );

    std::string bla;
    iar( bla );
    assert( bla == "bla" );

    std::shared_ptr<int> intptr;
    iar( CEREAL_NVP(intptr) );
    assert( *intptr == 99 );

    std::map<std::string, int> map1;

    iar( CEREAL_NVP(map1) );
    assert( map1["one"]   == 1 );
    assert( map1["two"]   == 2 );
    assert( map1["three"] == 3 );


    int x;
    iar( CEREAL_NVP(x) );
    assert( x == 3 );

    int x5;
    iar( x5 );
    assert( x5 == 5 );

    double x33;
    iar( x33 );
    assert( x33 == 3.3 );

    float x32;
    iar( x32 );
    assert( x32 == 3.2f );

    bool xtrue;
    iar( xtrue );
    assert( xtrue == true );

    std::array<int,5> arr;
    iar( arr );
    for( int i = 0; i < 5; ++i )
      assert( arr[i] == (i+1) );

    Everything e;
    iar( cereal::make_nvp("EVERYTHING", e) );
    //assert( e == e_out );

    std::vector<std::string> vec;
    iar( vec );
    assert( vec[0] == "hey" );
    assert( vec[1] == "there" );
    assert( vec[2] == "buddy" );

    std::vector<std::vector<std::string>> vec2;
    iar( vec2 );
    for( auto & v : vec2 )
    {
      assert( v[0] == "hey" );
      assert( v[1] == "there" );
      assert( v[2] == "buddy" );
    }

    int xxx[3];
    iar.loadBinaryValue( xxx, sizeof(int)*3 );
    assert( xxx[0] == -1 );
    assert( xxx[1] == 95 );
    assert( xxx[2] == 3 );

    std::unique_ptr<Derived> d1;
    std::unique_ptr<Base> d2;
    std::shared_ptr<Base> d3;

    iar( d1 );
    assert( d1->x == 4 && d1->y == 3 );
    iar( d2 );
    assert( dynamic_cast<Derived*>(d2.get())->x == 5 && dynamic_cast<Derived*>(d2.get())->y == 4 );
    iar( d3 );
    assert( dynamic_cast<Derived*>(d3.get())->x == 6 && dynamic_cast<Derived*>(d3.get())->y == 5 );
  }

  {
    std::ofstream b("endian.out", std::ios::binary);
    cereal::PortableBinaryOutputArchive oar(b);

    bool bb = true;
    char a = 'a';
    int x = 1234;
    float y = 1.324f;
    double z = 3.1452;
    long double d = 1.123451234512345;
    long long j = 2394873298472343;

    oar( bb, a, x, y, z, d, j );
    std::cout << bb << " " << a << " " << x << " " << y << " " << z << " " << d << " " << j << std::endl;
    // valgrind will complain about uninitialized bytes here - seems to be the padding caused by the long double and
    // long long allocations (this padding just exists on the stack and is never used anywhere)
    // see https://bugs.kde.org/show_bug.cgi?id=197915
  }
  {
    std::ifstream b("endian.out", std::ios::binary);
    cereal::PortableBinaryInputArchive iar(b);

    bool bb;
    char a;
    int x;
    float y;
    double z;
    long double d;
    long long j;

    iar( bb, a, x, y, z, d, j );

    std::cout << bb << " " << a << " " << x << " " << y << " " << z << " " << d << " " << j << std::endl;

    std::remove("endian.out");
  }

  {
    std::ofstream ss("xml_ordering.out");
    cereal::XMLOutputArchive ar(ss);

    double one = 1;
    double two = 2;
    double three = 3;
    std::vector<int> four = {1, 2, 3, 4};

    // Output is ordered 3 2 1 4
    ar( three, CEREAL_NVP(two), one, cereal::make_nvp("five", four) );
  }

  {
    std::ifstream ss("xml_ordering.out");
    cereal::XMLInputArchive ar(ss);

    // Output prodered out of order, try to load in order 1 2 3 4
    double one;
    double two;
    double three;
    std::vector<int> four;

    ar( one ); // cereal can only give warnings if you used an NVP!
    ar( CEREAL_NVP( two ) );
    ar( three );

    try
    {
      ar( CEREAL_NVP( three ) );
    }
    catch( cereal::Exception const & e )
    {
      std::cout << e.what() << std::endl;
      std::cout << "Looked for three but we didn't use an NVP when saving" << std::endl;
    }
    ar( cereal::make_nvp("five", four) );
    ar( cereal::make_nvp("five", four) ); // do it a second time since it shouldn't matter as we provide the name

    std::cout << one << std::endl;
    std::cout << two << std::endl;
    std::cout << three << std::endl;
    for( auto i : four ) std::cout << i << " ";
    std::cout << std::endl;
  }

  {
    // Boost transition layer stuff
    std::ofstream ss("cereal_version.out");
    cereal::XMLOutputArchive ar(ss);

    BoostTransitionMS b(3);
    ar( b, b );

    BoostTransitionSplit c(4);
    ar( c, c );

    BoostTransitionNMS d(5);
    ar( d, d );

    BoostTransitionNMSplit e(32);
    ar( e, e );
  }

  {
    // Boost transition layer stuff
    std::ifstream ss("cereal_version.out");
    cereal::XMLInputArchive ar(ss);

    BoostTransitionMS b;
    ar( b );
    assert( b.getX() == 3 );
    b.setX( 0 );
    ar( b );
    assert( b.getX() == 3 );

    BoostTransitionSplit c;
    ar( c );
    assert( c.getX() == 4 );
    c.setX( 0 );
    ar( c );
    assert( c.getX() == 4 );

    BoostTransitionNMS d;
    ar( d );
    assert( d.x == 5 );
    d.x = 0;
    ar( d );
    assert( d.x == 5 );

    BoostTransitionNMSplit e;
    ar( e );
    assert( e.x == 32 );
    e.x = 0;
    ar( e );
    assert( e.x == 32 );
  }

#ifdef CEREAL_FUTURE_EXPERIMENTAL
  {
    // Any testing
    int x = 32;
    int * xx = &x;
    std::string y("hello");
    cereal::detail::Any a(xx);
    auto b = a;

    std::cout << *((int *)a) << std::endl;
    *((int*)a) = 44;
    std::cout << *((int *)b) << std::endl;
    std::cout << *((int *)a) << std::endl;

    a = cereal::detail::Any(y);
    std::string a_out = a;
    std::cout << a_out << std::endl;
  }
#endif // CEREAL_FUTURE_EXPERIMENTAL*/

  return 0;
}

//CEREAL_CLASS_VERSION(BoostTransitionMS, 1)
//CEREAL_CLASS_VERSION(BoostTransitionSplit, 2)
//CEREAL_CLASS_VERSION(BoostTransitionNMS, 3)
// keep the other at default version (0)
