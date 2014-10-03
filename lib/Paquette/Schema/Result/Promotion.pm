package Paquette::Schema::Result::Promotion;

use strict;
use warnings;

use base 'DBIx::Class';

__PACKAGE__->load_components("InflateColumn::DateTime", "TimeStamp", "EncodedColumn", "Core");
__PACKAGE__->table("promotion");
__PACKAGE__->add_columns(
  "id",
  { data_type => "INT", default_value => undef, is_nullable => 0, size => 11 },
  "code",
  {
    data_type => "VARCHAR",
    default_value => undef,
    is_nullable => 0,
    size => 10,
  },
  "amount",
  { data_type => "DECIMAL", default_value => undef, is_nullable => 0, size => 5 },
  "over",
  { data_type => "DECIMAL", default_value => undef, is_nullable => 0, size => 5 },
  "type",
  {
    data_type => "VARCHAR",
    default_value => undef,
    is_nullable => 0,
    size => 10,
  },
  "description",
  {
    data_type => "VARCHAR",
    default_value => undef,
    is_nullable => 1,
    size => 128,
  },
);
__PACKAGE__->set_primary_key("id");


# Created by DBIx::Class::Schema::Loader v0.04005 @ 2010-04-12 12:18:36
# DO NOT MODIFY THIS OR ANYTHING ABOVE! md5sum:LClPx7EU0eOZYbIhktQsOg


# You can replace this text with custom content, and it will be preserved on regeneration
1;
