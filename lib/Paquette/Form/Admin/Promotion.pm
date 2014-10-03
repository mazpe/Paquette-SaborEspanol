package Paquette::Form::Admin::Promotion;
use HTML::FormHandler::Moose;
use HTML::FormHandler::Types ( 'NoSpaces', 'WordChars' );
extends 'HTML::FormHandler::Model::DBIC';
use Data::Dumper;

with 'HTML::FormHandler::Render::Simple'; # if you want to render the form

has '+item_class' => ( default => 'Promotion' );

has_field 'code'        => ( 
    type                => 'Text', 
    label               => 'Code', 
    required            => 1,
    required_message    => 'You must enter a code', 
    css_class           => 'form_col_a',
);
has_field 'type' => (
    type                => 'Select',
    label               => 'Type',
    required            => 0,
    required_message    => 'You must select active or inactive ',
    css_class           => 'form_col_a',
    options             => [{ value => 'percent', label => 'percent'},
                            { value => 'discount', label => 'discount'},
                            { value => 'over', label => 'over'},
                            { value => 'shipping', label => 'shipping'},]
);
has_field 'amount'        => (
    type                => 'Text',
    label               => 'Amount',
    required            => 1,
    required_message    => 'You must enter an amount',
    css_class           => 'form_col_a',
);
has_field 'over'        => (
    type                => 'Text',
    label               => 'Over',
    required            => 0,
    required_message    => 'You must enter an amount',
    css_class           => 'form_col_a',
);

has_field 'description'        => (
    type                => 'Text',
    label               => 'Description',
    css_class           => 'form_col_a',
);

has_field 'submit' => ( type => 'Submit', value => 'Submit' );


no HTML::FormHandler::Moose;
1;
