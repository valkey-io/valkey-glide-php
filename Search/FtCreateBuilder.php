<?php

/** Copyright Valkey GLIDE Project Contributors - SPDX Identifier: Apache-2.0 */

declare(strict_types=1);

namespace ValkeyGlide\Search;

/**
 * Builder for FT.CREATE commands.
 *
 * Collects index-level options and field definitions, then pass
 * the builder to the client's ftCreate() method.
 *
 * Usage:
 *   $client->ftCreate(
 *       (new FtCreateBuilder())
 *           ->index('myindex')
 *           ->on('HASH')
 *           ->prefix(['product:'])
 *           ->addField((new FtTextField('title'))->sortable())
 *           ->addField((new FtNumericField('price'))->sortable())
 *           ->addField(FtVectorField::hnsw('embedding', 1536, 'COSINE')->m(40))
 *   );
 *
 * @see \ValkeyGlide::ftCreate()
 * @see https://valkey.io/commands/ft.create/
 */
class FtCreateBuilder
{
    private ?string $indexName = null;
    private ?string $on = null;
    private array $prefixes = [];
    private ?float $score = null;
    private ?string $language = null;
    private bool $skipInitialScan = false;
    private ?int $minStemSize = null;
    private bool $noOffsets = false;
    private bool $noStopWords = false;
    private ?array $stopWords = null;
    private ?string $punctuation = null;

    /** @var array<FtTextField|FtNumericField|FtTagField|FtVectorField> */
    private array $fields = [];

    /** Set the index name. */
    public function index(string $name): self
    {
        $this->indexName = $name;
        return $this;
    }

    /** Set the data structure type to index ('HASH' or 'JSON'). */
    public function on(string $on): self
    {
        $this->on = strtoupper($on);
        return $this;
    }

    /** Set key prefixes to index. */
    public function prefix(array $prefixes): self
    {
        $this->prefixes = $prefixes;
        return $this;
    }

    /** Set the default score for documents. */
    public function score(float $score): self
    {
        $this->score = $score;
        return $this;
    }

    /** Set the default stemming language. */
    public function language(string $language): self
    {
        $this->language = $language;
        return $this;
    }

    /** Skip scanning existing keys when creating the index. */
    public function skipInitialScan(bool $skip = true): self
    {
        $this->skipInitialScan = $skip;
        return $this;
    }

    /** Set the minimum word length for stemming. */
    public function minStemSize(int $size): self
    {
        $this->minStemSize = $size;
        return $this;
    }

    /**
     * Disable storing term offsets.
     *
     * WITHOFFSETS is the default; use this method to disable offsets.
     */
    public function noOffsets(bool $noOffsets = true): self
    {
        $this->noOffsets = $noOffsets;
        return $this;
    }

    /** Disable stop-word filtering. */
    public function noStopWords(bool $noStopWords = true): self
    {
        $this->noStopWords = $noStopWords;
        return $this;
    }

    /** Set a custom stop-word list. */
    public function stopWords(array $words): self
    {
        $this->stopWords = $words;
        return $this;
    }

    /** Set custom word separator characters. */
    public function punctuation(string $punctuation): self
    {
        $this->punctuation = $punctuation;
        return $this;
    }

    /**
     * Add a field to the schema.
     *
     * @param FtTextField|FtNumericField|FtTagField|FtVectorField $field
     */
    public function addField(FtTextField|FtNumericField|FtTagField|FtVectorField $field): self
    {
        $this->fields[] = $field;
        return $this;
    }

    /**
     * Convert the builder state into the arguments for ftCreate().
     *
     * Returns ['index' => string, 'schema' => array, 'options' => array|null].
     *
     * @internal Called by the C layer via ftCreate().
     * @return array
     * @throws \ValkeyGlideException if index name is not set or no fields are defined.
     */
    public function toArray(): array
    {
        if ($this->indexName === null) {
            throw new \ValkeyGlideException('FtCreateBuilder: index name is required');
        }
        if (empty($this->fields)) {
            throw new \ValkeyGlideException('FtCreateBuilder: at least one field is required');
        }

        $schema = [];
        foreach ($this->fields as $field) {
            $schema[] = $field->toArray();
        }

        $options = $this->buildOptions();

        return [
            'index'   => $this->indexName,
            'schema'  => $schema,
            'options' => empty($options) ? null : $options,
        ];
    }

    /** @internal Build the options array. */
    private function buildOptions(): array
    {
        $options = [];
        if ($this->on !== null) {
            $options['ON'] = $this->on;
        }
        if (!empty($this->prefixes)) {
            $options['PREFIX'] = $this->prefixes;
        }
        if ($this->score !== null) {
            $options['SCORE'] = $this->score;
        }
        if ($this->language !== null) {
            $options['LANGUAGE'] = $this->language;
        }
        if ($this->skipInitialScan) {
            $options['SKIPINITIALSCAN'] = true;
        }
        if ($this->minStemSize !== null) {
            $options['MINSTEMSIZE'] = $this->minStemSize;
        }
        if ($this->noOffsets) {
            $options['NOOFFSETS'] = true;
        }
        if ($this->noStopWords) {
            $options['NOSTOPWORDS'] = true;
        }
        if ($this->stopWords !== null) {
            $options['STOPWORDS'] = $this->stopWords;
        }
        if ($this->punctuation !== null) {
            $options['PUNCTUATION'] = $this->punctuation;
        }
        return $options;
    }
}
