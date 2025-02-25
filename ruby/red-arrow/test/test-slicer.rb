# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.

class SlicerTest < Test::Unit::TestCase
  def setup
    @count_field = Arrow::Field.new("count", :uint32)
    @visible_field = Arrow::Field.new("visible", :boolean)
    schema = Arrow::Schema.new([@count_field, @visible_field])
    count_arrays = [
      Arrow::UInt32Array.new([0, 1, 2]),
      Arrow::UInt32Array.new([4, 8, 16]),
      Arrow::UInt32Array.new([32, 64, nil]),
      Arrow::UInt32Array.new([256]),
    ]
    visible_arrays = [
      Arrow::BooleanArray.new([nil, true, false, nil]),
      Arrow::BooleanArray.new([true]),
      Arrow::BooleanArray.new([true, false]),
      Arrow::BooleanArray.new([nil]),
      Arrow::BooleanArray.new([nil]),
      Arrow::BooleanArray.new([true]),
    ]
    @count_array = Arrow::ChunkedArray.new(count_arrays)
    @visible_array = Arrow::ChunkedArray.new(visible_arrays)
    @table = Arrow::Table.new(schema, [@count_array, @visible_array])
  end

  sub_test_case("column") do
    test("BooleanArray") do
      sliced_table = @table.slice do |slicer|
        slicer.visible
      end
      assert_equal(<<-TABLE, sliced_table.to_s)
	 count	visible
0	(null)	 (null)
1	     1	true   
2	(null)	 (null)
3	     8	true   
4	    16	true   
5	(null)	 (null)
6	(null)	 (null)
7	   256	true   
      TABLE
    end

    test("not BooleanArray") do
      sliced_table = @table.slice do |slicer|
        slicer.count
      end
      assert_equal(<<-TABLE, sliced_table.to_s)
	 count	visible
0	     1	true   
1	     2	false  
2	     4	 (null)
3	     8	true   
4	    16	true   
5	    32	false  
6	    64	 (null)
7	(null)	 (null)
8	   256	true   
      TABLE
    end
  end

  sub_test_case("!column") do
    test("BooleanArray") do
      sliced_table = @table.slice do |slicer|
        !slicer.visible
      end
      assert_equal(<<-TABLE, sliced_table.to_s)
	 count	visible
0	(null)	 (null)
1	     2	false  
2	(null)	 (null)
3	    32	false  
4	(null)	 (null)
5	(null)	 (null)
      TABLE
    end

    test("not BooleanArray") do
      sliced_table = @table.slice do |slicer|
        !slicer.count
      end
      assert_equal(<<-TABLE, sliced_table.to_s)
	 count	visible
0	     0	 (null)
1	(null)	 (null)
      TABLE
    end
  end

  test("column.null?") do
    sliced_table = @table.slice do |slicer|
      slicer.visible.null?
    end
    assert_equal(<<-TABLE, sliced_table.to_s)
	 count	visible
0	     0	 (null)
1	     4	 (null)
2	    64	 (null)
3	(null)	 (null)
    TABLE
  end

  test("column.valid?") do
    sliced_table = @table.slice do |slicer|
      slicer.visible.valid?
    end
    assert_equal(<<-TABLE, sliced_table.to_s)
	count	visible
0	    1	true   
1	    2	false  
2	    8	true   
3	   16	true   
4	   32	false  
5	  256	true   
    TABLE
  end

  sub_test_case("column ==") do
    test("nil") do
      sliced_table = @table.slice do |slicer|
        slicer.visible == nil
      end
      assert_equal(<<-TABLE, sliced_table.to_s)
	 count	visible
0	     0	 (null)
1	     4	 (null)
2	    64	 (null)
3	(null)	 (null)
      TABLE
    end

    test("value") do
      sliced_table = @table.slice do |slicer|
        slicer.visible == true
      end
      assert_equal(<<-TABLE, sliced_table.to_s)
	 count	visible
0	(null)	 (null)
1	     1	true   
2	(null)	 (null)
3	     8	true   
4	    16	true   
5	(null)	 (null)
6	(null)	 (null)
7	   256	true   
      TABLE
    end
  end

  sub_test_case("!(column ==)") do
    test("nil") do
      sliced_table = @table.slice do |slicer|
        !(slicer.visible == nil)
      end
      assert_equal(<<-TABLE, sliced_table.to_s)
	count	visible
0	    1	true   
1	    2	false  
2	    8	true   
3	   16	true   
4	   32	false  
5	  256	true   
      TABLE
    end

    test("value") do
      sliced_table = @table.slice do |slicer|
        !(slicer.visible == true)
      end
      assert_equal(<<-TABLE, sliced_table.to_s)
	 count	visible
0	(null)	 (null)
1	     2	false  
2	(null)	 (null)
3	    32	false  
4	(null)	 (null)
5	(null)	 (null)
      TABLE
    end
  end

  sub_test_case("column !=") do
    test("nil") do
      sliced_table = @table.slice do |slicer|
        slicer.visible != nil
      end
      assert_equal(<<-TABLE, sliced_table.to_s)
	count	visible
0	    1	true   
1	    2	false  
2	    8	true   
3	   16	true   
4	   32	false  
5	  256	true   
      TABLE
    end

    test("value") do
      sliced_table = @table.slice do |slicer|
        slicer.visible != true
      end
      assert_equal(<<-TABLE, sliced_table.to_s)
	 count	visible
0	(null)	 (null)
1	     2	false  
2	(null)	 (null)
3	    32	false  
4	(null)	 (null)
5	(null)	 (null)
      TABLE
    end
  end

  test("column < value") do
    sliced_table = @table.slice do |slicer|
      slicer.count < 16
    end
    assert_equal(<<-TABLE, sliced_table.to_s)
	 count	visible
0	     0	 (null)
1	     1	true   
2	     2	false  
3	     4	 (null)
4	     8	true   
5	(null)	 (null)
    TABLE
  end

  test("!(column < value)") do
    sliced_table = @table.slice do |slicer|
      !(slicer.count < 16)
    end
    assert_equal(<<-TABLE, sliced_table.to_s)
	 count	visible
0	    16	true   
1	    32	false  
2	    64	 (null)
3	(null)	 (null)
4	   256	true   
    TABLE
  end

  test("column <= value") do
    sliced_table = @table.slice do |slicer|
      slicer.count <= 16
    end
    assert_equal(<<-TABLE, sliced_table.to_s)
	 count	visible
0	     0	 (null)
1	     1	true   
2	     2	false  
3	     4	 (null)
4	     8	true   
5	    16	true   
6	(null)	 (null)
    TABLE
  end

  test("!(column <= value)") do
    sliced_table = @table.slice do |slicer|
      !(slicer.count <= 16)
    end
    assert_equal(<<-TABLE, sliced_table.to_s)
	 count	visible
0	    32	false  
1	    64	 (null)
2	(null)	 (null)
3	   256	true   
    TABLE
  end

  test("column > value") do
    sliced_table = @table.slice do |slicer|
      slicer.count > 16
    end
    assert_equal(<<-TABLE, sliced_table.to_s)
	 count	visible
0	    32	false  
1	    64	 (null)
2	(null)	 (null)
3	   256	true   
    TABLE
  end

  test("!(column > value)") do
    sliced_table = @table.slice do |slicer|
      !(slicer.count > 16)
    end
    assert_equal(<<-TABLE, sliced_table.to_s)
	 count	visible
0	     0	 (null)
1	     1	true   
2	     2	false  
3	     4	 (null)
4	     8	true   
5	    16	true   
6	(null)	 (null)
    TABLE
  end

  test("column >= value") do
    sliced_table = @table.slice do |slicer|
      slicer.count >= 16
    end
    assert_equal(<<-TABLE, sliced_table.to_s)
	 count	visible
0	    16	true   
1	    32	false  
2	    64	 (null)
3	(null)	 (null)
4	   256	true   
    TABLE
  end

  test("!(column >= value)") do
    sliced_table = @table.slice do |slicer|
      !(slicer.count >= 16)
    end
    assert_equal(<<-TABLE, sliced_table.to_s)
	 count	visible
0	     0	 (null)
1	     1	true   
2	     2	false  
3	     4	 (null)
4	     8	true   
5	(null)	 (null)
    TABLE
  end

  test("column.in") do
    sliced_table = @table.slice do |slicer|
      slicer.count.in?([1, 4, 16, 64])
    end
    assert_equal(<<-TABLE, sliced_table.to_s)
	 count	visible
0	     1	true   
1	     4	 (null)
2	    16	true   
3	    64	 (null)
4	(null)	 (null)
    TABLE
  end

  test("!column.in") do
    sliced_table = @table.slice do |slicer|
      !slicer.count.in?([1, 4, 16, 64])
    end
    assert_equal(<<-TABLE, sliced_table.to_s)
	 count	visible
0	     0	 (null)
1	     2	false  
2	     8	true   
3	    32	false  
4	(null)	 (null)
5	   256	true   
    TABLE
  end

  test("condition & condition") do
    sliced_table = @table.slice do |slicer|
      slicer.visible & (slicer.count >= 16)
    end
    assert_equal(<<-TABLE, sliced_table.to_s)
	 count	visible
0	(null)	 (null)
1	(null)	 (null)
2	    16	true   
3	(null)	 (null)
4	(null)	 (null)
5	   256	true   
    TABLE
  end

  test("condition | condition") do
    sliced_table = @table.slice do |slicer|
      slicer.visible | (slicer.count >= 16)
    end
    assert_equal(<<-TABLE, sliced_table.to_s)
	 count	visible
0	(null)	 (null)
1	     1	true   
2	(null)	 (null)
3	     8	true   
4	    16	true   
5	    32	false  
6	(null)	 (null)
7	(null)	 (null)
8	   256	true   
    TABLE
  end

  test("condition ^ condition") do
    sliced_table = @table.slice do |slicer|
      slicer.visible ^ (slicer.count >= 16)
    end
    assert_equal(<<-TABLE, sliced_table.to_s)
	 count	visible
0	(null)	 (null)
1	     1	true   
2	(null)	 (null)
3	     8	true   
4	    32	false  
5	(null)	 (null)
6	(null)	 (null)
    TABLE
  end

  test("select") do
    sliced_table = @table.slice do |slicer|
      slicer.visible.select do |value|
        value.nil? or value
      end
    end
    assert_equal(<<-TABLE, sliced_table.to_s)
	 count	visible
0	     0	 (null)
1	     1	true   
2	     4	 (null)
3	     8	true   
4	    16	true   
5	    64	 (null)
6	(null)	 (null)
7	   256	true   
    TABLE
  end

  test("!select") do
    sliced_table = @table.slice do |slicer|
      !slicer.visible.select do |value|
        value.nil? or value
      end
    end
    assert_equal(<<-TABLE, sliced_table.to_s)
	count	visible
0	    2	false  
1	   32	false  
    TABLE
  end

  test("reject") do
    sliced_table = @table.slice do |slicer|
      slicer.visible.reject do |value|
        value.nil? or value
      end
    end
    assert_equal(<<-TABLE, sliced_table.to_s)
	count	visible
0	    2	false  
1	   32	false  
    TABLE
  end

  test("!reject") do
    sliced_table = @table.slice do |slicer|
      !slicer.visible.reject do |value|
        value.nil? or value
      end
    end
    assert_equal(<<-TABLE, sliced_table.to_s)
	 count	visible
0	     0	 (null)
1	     1	true   
2	     4	 (null)
3	     8	true   
4	    16	true   
5	    64	 (null)
6	(null)	 (null)
7	   256	true   
    TABLE
  end
end
