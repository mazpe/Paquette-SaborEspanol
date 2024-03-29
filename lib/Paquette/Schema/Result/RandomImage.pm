package Paquette::Schema::Result::RandomImage;

use strict;
use warnings;

use base 'DBIx::Class';

__PACKAGE__->load_components("InflateColumn::DateTime", "TimeStamp", "EncodedColumn", "Core");
__PACKAGE__->table("random_image");
__PACKAGE__->add_columns(
  "name",
  {
    data_type => "VARCHAR",
    default_value => undef,
    is_nullable => 0,
    size => 32,
  },
);


# Created by DBIx::Class::Schema::Loader v0.04005 @ 2010-04-12 12:18:36
# DO NOT MODIFY THIS OR ANYTHING ABOVE! md5sum:en4OhWTvWnXhBlDbMU+z/g


# You can replace this text with custom content, and it will be preserved on regeneration
1;
