package Paquette::Form::Admin::State;
use HTML::FormHandler::Moose;
use HTML::FormHandler::Types ( 'NoSpaces', 'WordChars' );
extends 'HTML::FormHandler::Model::DBIC';
use Data::Dumper;

with 'HTML::FormHandler::Render::Simple'; # if you want to render the form

has '+item_class' => ( default => 'State' );

has_field 'name'        => ( 
    type                => 'Text', 
    label               => 'Name', 
    required            => 1,
    required_message    => 'You must enter a state', 
    css_class           => 'form_col_a',
);
has_field 'abbr'        => (
    type                => 'Text',
    label               => 'Abbr',
    required            => 1,
    required_message    => 'You must enter an abbr',
    css_class           => 'form_col_a',
);

has_field 'submit' => ( type => 'Submit', value => 'Submit' );

no HTML::FormHandler::Moose;
1;
